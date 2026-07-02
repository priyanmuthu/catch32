import { exec } from "node:child_process";

const STATUSES = new Set(["available", "focus", "in_meeting"]);

export class MeetingController {
  constructor(config, logger = console) {
    this.config = config.meeting ?? {};
    this.status = this.config.status ?? "available";
    this.muted = Boolean(this.config.muted);
    this.logger = logger;
  }

  handlePress(action) {
    if (action === "toggle_mute") {
      this.muted = !this.muted;
      this.runToggleMuteCommand();
    } else if (STATUSES.has(action)) {
      this.status = action;
    }
    return this.state();
  }

  runToggleMuteCommand() {
    const commandConfig = this.config.toggleMuteCommand;
    if (!commandConfig?.command) {
      return;
    }
    if (!commandConfig.enabled) {
      this.logger.log(`[dry-run] ${commandConfig.command}`);
      return;
    }
    exec(commandConfig.command, (error, stdout, stderr) => {
      if (stdout.trim()) {
        this.logger.log(stdout.trim());
      }
      if (stderr.trim()) {
        this.logger.error(stderr.trim());
      }
      if (error) {
        this.logger.error(error.message);
      }
    });
  }

  state() {
    return {
      type: "state.meeting",
      status: this.status,
      muted: this.muted,
    };
  }
}

