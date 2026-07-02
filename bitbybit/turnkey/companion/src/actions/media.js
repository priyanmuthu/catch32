export class MediaController {
  constructor(config) {
    this.profile = config.commandProfiles?.[0] ?? { id: "audio", label: "Audio", step: 2 };
    this.value = 50;
    this.muted = false;
  }

  handleRotate(delta) {
    const step = Number(this.profile.step ?? 2);
    this.value = Math.max(0, Math.min(100, this.value + delta * step));
    return this.state();
  }

  handlePress(action) {
    if (action === "toggle_mute") {
      this.muted = !this.muted;
    }
    return this.state();
  }

  state() {
    return {
      type: "state.command",
      profile: this.profile.id,
      value: this.value,
      muted: this.muted,
    };
  }
}

