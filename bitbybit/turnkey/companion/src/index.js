import fs from "node:fs/promises";
import path from "node:path";
import process from "node:process";

import { createLineParser } from "./protocol.js";
import { openDevice, send } from "./serial.js";
import { MacroRunner } from "./actions/macros.js";
import { MediaController } from "./actions/media.js";
import { MeetingController } from "./actions/meeting.js";

const VERSION = "0.1.0";

function parseArgs(argv) {
  const args = {
    config: "config.example.json",
    port: undefined,
  };

  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--config") {
      args.config = argv[++i];
    } else if (argv[i] === "--port") {
      args.port = argv[++i];
    } else if (argv[i] === "--help") {
      args.help = true;
    }
  }

  return args;
}

function printHelp() {
  console.log("Usage: npm start -- [--config config.local.json] [--port /dev/cu.usbmodemXXXX]");
}

async function loadConfig(configPath) {
  const resolvedPath = path.resolve(process.cwd(), configPath);
  const raw = await fs.readFile(resolvedPath, "utf8");
  return {
    config: JSON.parse(raw),
    configPath: resolvedPath,
    configDir: path.dirname(resolvedPath),
  };
}

async function main() {
  const args = parseArgs(process.argv.slice(2));
  if (args.help) {
    printHelp();
    return;
  }

  const { config, configPath, configDir } = await loadConfig(args.config);
  const baudRate = Number(config.serial?.baud ?? 115200);
  const port = await openDevice({ path: args.port, baudRate });
  const media = new MediaController(config);
  const meeting = new MeetingController(config);
  const macros = new MacroRunner(config, configDir);

  console.log(`TurnKey companion ${VERSION}`);
  console.log(`Config: ${configPath}`);
  console.log(`Serial: ${port.path} @ ${baudRate}`);

  const sendState = (message) => {
    console.log("host>", message);
    send(port, message);
  };

  sendState({ type: "hello", app: "turnkey-companion", version: VERSION });
  sendState(media.state());
  sendState(meeting.state());

  const onDeviceMessage = (message) => {
    console.log("device>", message);

    if (message.type === "hello") {
      sendState({ type: "hello", app: "turnkey-companion", version: VERSION });
      return;
    }

    if (message.type === "input.rotate" && message.mode === "command") {
      sendState(media.handleRotate(Number(message.delta ?? 0)));
      return;
    }

    if (message.type === "input.press" && message.mode === "command") {
      sendState(media.handlePress(message.action));
      return;
    }

    if (message.type === "input.press" && message.mode === "meeting") {
      sendState(meeting.handlePress(message.action));
      return;
    }

    if (message.type === "macro.run") {
      macros.run(message.id, sendState);
    }
  };

  port.on("data", createLineParser(onDeviceMessage));
  port.on("error", (error) => console.error(error.message));
  port.on("close", () => console.log("Serial port closed"));
}

main().catch((error) => {
  console.error(error.message);
  process.exitCode = 1;
});

