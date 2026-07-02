import { SerialPort } from "serialport";
import { encodeMessage } from "./protocol.js";

export async function listPorts() {
  return SerialPort.list();
}

export async function findDevicePort() {
  const ports = await listPorts();
  return ports.find((port) => {
    const text = `${port.path} ${port.manufacturer ?? ""} ${port.friendlyName ?? ""}`.toLowerCase();
    return text.includes("usb") || text.includes("esp32") || text.includes("wch") || text.includes("serial");
  });
}

export async function openDevice({ path, baudRate }) {
  const selectedPath = path ?? (await findDevicePort())?.path;
  if (!selectedPath) {
    throw new Error("No serial device found. Pass --port /dev/cu.usbmodemXXXX.");
  }

  const port = new SerialPort({
    path: selectedPath,
    baudRate,
    autoOpen: false,
  });

  await new Promise((resolve, reject) => {
    port.open((error) => (error ? reject(error) : resolve()));
  });

  return port;
}

export function send(port, message) {
  if (!port?.writable) {
    return;
  }
  port.write(encodeMessage(message));
}

