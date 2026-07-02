export function encodeMessage(message) {
  return `${JSON.stringify(message)}\n`;
}

export function createLineParser(onMessage, onError = console.error) {
  let buffer = "";

  return (chunk) => {
    buffer += chunk.toString("utf8");

    for (;;) {
      const newlineIndex = buffer.indexOf("\n");
      if (newlineIndex === -1) {
        break;
      }

      const line = buffer.slice(0, newlineIndex).trim();
      buffer = buffer.slice(newlineIndex + 1);
      if (!line) {
        continue;
      }

      try {
        onMessage(JSON.parse(line));
      } catch (error) {
        onError(new Error(`Invalid JSON from device: ${line}`, { cause: error }));
      }
    }
  };
}

