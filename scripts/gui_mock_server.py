#!/usr/bin/env python3

from __future__ import annotations

import argparse
import math
import signal
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import msgpack
import yaml
import zmq


MOTORS = ["XLeft", "XRight", "YLeft", "YRight", "ZLeft", "ZRight"]
ARMS = ["Left", "Right", "Gantry"]
HOMING_AXES = ["XLeft", "YLeft", "XRight", "YRight", "Z"]


def motor_flags(
    *,
    brake: str = "Off",
    enabled: str = "On",
    warning: str = "Off",
    alarm: str = "Off",
    homing: str = "Off",
) -> dict[str, str]:
    return {
        "BrakeApplied": brake,
        "Enabled": enabled,
        "Warning": warning,
        "Alarm": alarm,
        "Homing": homing,
    }


def empty_motor_status() -> dict[str, Any]:
    return {
        "currentPosition": 0.0,
        "targetPosition": 0.0,
        "speed": 0.0,
        "speedRpm": 0.0,
        "torque": 0,
        "state": "On",
        "warningDescription": "",
        "alarmDescription": "",
        "flags": motor_flags(),
        "speedCommandPercent": 0.0,
        "modeMaxLinearSpeedMmPerSec": 0.0,
    }


def empty_tool_changer() -> dict[str, dict[str, str]]:
    return {
        "flags": {
            "ProxSen": "On",
            "OpenSen": "Off",
            "ClosedSen": "On",
            "OpenValve": "Off",
            "ClosedValve": "On",
        }
    }


def axis_to_motor(axis: str) -> str:
    return "ZLeft" if axis == "Z" else axis


def load_addresses(config_path: Path) -> tuple[str, str]:
    data = yaml.safe_load(config_path.read_text())
    classes = data.get("classes", {})
    server = classes.get("RimoServer", {})
    return (
        server.get("statusAddress", "ipc:///tmp/rimoStatus"),
        server.get("commandAddress", "ipc:///tmp/rimoCommand"),
    )


@dataclass
class HomingStep:
    axis: str
    state: str = "Pending"
    message: str = ""
    started_at: float | None = None


@dataclass
class MockState:
    motor_enabled: dict[str, bool] = field(
        default_factory=lambda: {motor: True for motor in MOTORS}
    )
    alarms: dict[str, str] = field(default_factory=dict)
    warnings: dict[str, str] = field(default_factory=dict)
    session_state: str = "Idle"
    session_message: str = ""
    session_axes: list[HomingStep] = field(default_factory=list)
    session_index: int = 0
    positions: dict[str, float] = field(
        default_factory=lambda: {
            "XLeft": 1500.0,
            "XRight": 3000.0,
            "YLeft": -400.0,
            "YRight": -400.0,
            "ZLeft": 100.0,
            "ZRight": 100.0,
        }
    )
    lock: threading.Lock = field(default_factory=threading.Lock)

    def _start_axis(self, now: float) -> None:
        if self.session_index >= len(self.session_axes):
            self.session_state = "Succeeded"
            self.session_message = "Homing complete"
            return
        step = self.session_axes[self.session_index]
        step.state = "Running"
        step.message = "Homing in progress"
        step.started_at = now
        self.session_state = "Running"
        self.session_message = f"Homing {step.axis}..."

    def start_homing(self, target: str) -> None:
        now = time.monotonic()
        axes = HOMING_AXES if target == "All" else [target]
        self.session_axes = [HomingStep(axis=axis) for axis in axes]
        self.session_index = 0
        self.session_state = "Pending"
        self.session_message = ""
        self._start_axis(now)

    def tick_homing(self, timeout_s: float = 6.0, finish_s: float = 1.8) -> None:
        if self.session_state != "Running" or self.session_index >= len(self.session_axes):
            return
        step = self.session_axes[self.session_index]
        if step.started_at is None:
            return
        now = time.monotonic()
        elapsed = now - step.started_at
        motor = axis_to_motor(step.axis)
        if motor in self.alarms:
            step.state = "Failed"
            step.message = self.alarms[motor]
            self.session_state = "Failed"
            self.session_message = f"{step.axis}: {step.message}"
            return
        if elapsed >= timeout_s:
            step.state = "TimedOut"
            step.message = f"Timed out after {int(timeout_s * 1000)} ms."
            self.session_state = "TimedOut"
            self.session_message = f"{step.axis}: {step.message}"
            return
        if elapsed < finish_s:
            return
        step.state = "Succeeded"
        step.message = ""
        self.session_index += 1
        self._start_axis(now)

    def build_status(self) -> dict[str, Any]:
        self.tick_homing()
        t = time.monotonic()
        status = {
            "motors": {},
            "toolChangers": {arm: empty_tool_changer() for arm in ("Left", "Right")},
            "robotComponents": {
                "Contec": "On",
                "MotorControl": "Warning" if self.warnings else "On",
                "ControlPanel": "On",
            },
            "joystics": {
                "Left": {"x": math.sin(t * 0.8) * 0.2, "y": math.cos(t * 0.7) * 0.2, "btn": False},
                "Right": {"x": math.cos(t * 0.6) * 0.2, "y": math.sin(t * 0.9) * 0.2, "btn": False},
                "Gantry": {"x": 0.0, "y": math.sin(t * 0.4) * 0.15, "btn": False},
            },
            "safetyOn": True,
            "armStates": {"Left": "Slow", "Right": "Slow", "Gantry": "Slow"},
            "homing": {
                "state": self.session_state,
                "message": self.session_message,
                "axes": [
                    {"axis": step.axis, "state": step.state, "message": step.message}
                    for step in self.session_axes
                ],
            },
        }

        active_homing_motor = None
        if self.session_state == "Running" and self.session_index < len(self.session_axes):
            active_homing_motor = axis_to_motor(self.session_axes[self.session_index].axis)

        for i, motor in enumerate(MOTORS):
            phase = t * (0.6 + i * 0.07)
            position = self.positions[motor] + math.sin(phase) * (120.0 if "Z" not in motor else 20.0)
            speed = math.cos(phase) * (35.0 if "Z" not in motor else 12.0)
            motor_state = empty_motor_status()
            motor_state["currentPosition"] = position
            motor_state["targetPosition"] = self.positions[motor]
            motor_state["speed"] = speed
            motor_state["speedRpm"] = speed * 1.4
            motor_state["torque"] = int(abs(speed) * 2)
            motor_state["modeMaxLinearSpeedMmPerSec"] = 80.0 if "Z" not in motor else 150.0
            motor_state["speedCommandPercent"] = max(-100.0, min(100.0, speed * 2))
            motor_state["flags"]["Enabled"] = "On" if self.motor_enabled[motor] else "Warning"
            if motor in self.warnings:
                motor_state["state"] = "Warning"
                motor_state["flags"]["Warning"] = "Warning"
                motor_state["warningDescription"] = self.warnings[motor]
            if motor in self.alarms:
                motor_state["state"] = "Error"
                motor_state["flags"]["Alarm"] = "Error"
                motor_state["alarmDescription"] = self.alarms[motor]
            if motor == active_homing_motor and self.session_state == "Running":
                motor_state["flags"]["Homing"] = "Warning"
            elif any(
                axis_to_motor(step.axis) == motor and step.state == "Succeeded"
                for step in self.session_axes
            ):
                motor_state["flags"]["Homing"] = "On"
            status["motors"][motor] = motor_state
        return status


def ok_response(message: str = "", payload: dict[str, Any] | None = None) -> dict[str, Any]:
    response: dict[str, Any] = {"status": "OK", "message": message}
    if payload is not None:
        response["response"] = payload
    return response


def error_response(message: str) -> dict[str, Any]:
    return {"status": "Error", "message": message}


def handle_command(state: MockState, command: dict[str, Any]) -> dict[str, Any]:
    cmd_type = command.get("type")
    if not isinstance(cmd_type, str):
        return error_response("Command lacks a valid 'type' entry")

    with state.lock:
        if cmd_type == "toolChanger":
            position = command.get("position")
            action = command.get("action")
            if position not in ("Left", "Right") or action not in ("Open", "Close"):
                return error_response("toolChanger requires valid 'position' and 'action'")
            flags = empty_tool_changer()["flags"]
            open_on = action == "Open"
            if position == "Left":
                pass
            return ok_response()
        if cmd_type == "reset":
            system = command.get("system")
            if system == "MotorControl":
                state.alarms.clear()
                state.warnings.clear()
            return ok_response()
        if cmd_type == "setAllMotorsEnabled":
            enabled = bool(command.get("enabled"))
            for motor in MOTORS:
                state.motor_enabled[motor] = enabled
            return ok_response()
        if cmd_type == "setMotorEnabled":
            motor = command.get("motor")
            if motor not in MOTORS:
                return error_response("Unknown motor")
            state.motor_enabled[motor] = bool(command.get("enabled"))
            return ok_response()
        if cmd_type == "resetMotorAlarm":
            motor = command.get("motor")
            state.alarms.pop(motor, None)
            return ok_response()
        if cmd_type == "motorDiagnostics":
            motor = command.get("motor")
            if motor not in MOTORS:
                return error_response("Unknown motor")
            return ok_response(
                payload={
                    "motor": motor,
                    "driverInputRaw": 0,
                    "driverOutputRaw": 0,
                    "ioOutputRaw": 0,
                    "ioInputRaw": 0,
                    "netInputRaw": 0,
                    "netOutputRaw": 0,
                    "inputFlags": [],
                    "outputFlags": [],
                    "ioOutputAssignments": [],
                    "ioInputAssignments": [],
                    "netOutputAssignments": [],
                    "netInputAssignments": [],
                    "alarm": {},
                    "warning": {},
                },
            )
        if cmd_type == "contecDiagnostics":
            return ok_response(
                payload={
                    "inputsRaw": [True, False, True, False],
                    "outputsRaw": [False, False, True, False],
                    "inputs": [],
                    "outputs": [],
                }
            )
        if cmd_type == "homing":
            target = command.get("target")
            if target not in ("All", "XLeft", "YLeft", "XRight", "YRight", "Z"):
                return error_response("Unknown homing target")
            state.start_homing(target)
            return ok_response()
        if cmd_type == "emergencyStop":
            for motor in MOTORS:
                state.motor_enabled[motor] = False
            state.session_state = "Failed"
            state.session_message = "Emergency stop triggered"
            if state.session_index < len(state.session_axes):
                step = state.session_axes[state.session_index]
                step.state = "Failed"
                step.message = "Emergency stop triggered"
            return ok_response()
        return error_response(f"Unsupported command type: {cmd_type}")


def publisher_loop(pub_socket: zmq.Socket, state: MockState, stop_event: threading.Event, rate_hz: float) -> None:
    interval = 1.0 / rate_hz
    while not stop_event.is_set():
        with state.lock:
            payload = state.build_status()
        pub_socket.send(msgpack.packb(payload, use_bin_type=True))
        stop_event.wait(interval)


def command_loop(rep_socket: zmq.Socket, state: MockState, stop_event: threading.Event) -> None:
    while not stop_event.is_set():
        try:
            payload = rep_socket.recv(flags=zmq.NOBLOCK)
        except zmq.Again:
            time.sleep(0.02)
            continue

        try:
            command = msgpack.unpackb(payload, raw=False)
            if not isinstance(command, dict):
                response = error_response("Command must be a map")
            else:
                response = handle_command(state, command)
        except Exception as exc:  # pragmatic mock
            response = error_response(str(exc))
        rep_socket.send(msgpack.packb(response, use_bin_type=True))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Mock server for the Rimokun GUI")
    parser.add_argument(
        "-c",
        "--config",
        default="Config/rimokun.yaml",
        help="Path to rimokun YAML config",
    )
    parser.add_argument(
        "--rate",
        type=float,
        default=10.0,
        help="Status publish rate in Hz",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    config_path = Path(args.config)
    status_address, command_address = load_addresses(config_path)

    ctx = zmq.Context.instance()
    pub_socket = ctx.socket(zmq.PUB)
    rep_socket = ctx.socket(zmq.REP)
    pub_socket.bind(status_address)
    rep_socket.bind(command_address)

    state = MockState()
    stop_event = threading.Event()

    def request_stop(*_args: object) -> None:
        stop_event.set()

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    publisher = threading.Thread(
        target=publisher_loop,
        args=(pub_socket, state, stop_event, args.rate),
        daemon=True,
    )
    commands = threading.Thread(
        target=command_loop,
        args=(rep_socket, state, stop_event),
        daemon=True,
    )
    publisher.start()
    commands.start()

    print(f"Mock GUI server listening on status={status_address} command={command_address}")
    try:
        while not stop_event.is_set():
            time.sleep(0.2)
    finally:
        stop_event.set()
        publisher.join(timeout=1.0)
        commands.join(timeout=1.0)
        pub_socket.close(0)
        rep_socket.close(0)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
