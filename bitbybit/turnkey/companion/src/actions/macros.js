import { exec } from "node:child_process";
import path from "node:path";

export class MacroRunner {
  constructor(config, baseDir, logger = console) {
    this.macros = new Map((config.macros ?? []).map((macro) => [macro.id, macro]));
    this.baseDir = baseDir;
    this.logger = logger;
  }

  run(id, onState) {
    const macro = this.macros.get(id);
    if (!macro) {
      onState({ type: "error", message: `Unknown macro: ${id}` });
      return;
    }

    onState({ type: "state.macro", id, status: "running", label: `${macro.label} running` });

    if (!macro.enabled) {
      this.logger.log(`[dry-run] ${macro.command}`);
      onState({ type: "state.macro", id, status: "ok", label: `${macro.label} dry-run` });
      return;
    }

    const cwd = macro.cwd ? path.resolve(this.baseDir, macro.cwd) : this.baseDir;
    exec(macro.command, { cwd }, (error, stdout, stderr) => {
      if (stdout.trim()) {
        this.logger.log(stdout.trim());
      }
      if (stderr.trim()) {
        this.logger.error(stderr.trim());
      }
      if (error) {
        onState({ type: "state.macro", id, status: "failed", label: `${macro.label} failed` });
        this.logger.error(error.message);
        return;
      }
      onState({ type: "state.macro", id, status: "ok", label: `${macro.label} done` });
    });
  }
}

