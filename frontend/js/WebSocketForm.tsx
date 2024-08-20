import React, { FormEvent, useMemo, useState } from "react";

const WebSocketForm = ({ wsUrl }: { wsUrl: string }) => {
  const [inputValue, setInputValue] = useState("");
  const ws = useMemo(() => new WebSocket(wsUrl), [wsUrl]);

  ws.onopen = () => {
    console.log("WebSocket connected");
  };

  ws.onclose = () => {
    console.log("WebSocket disconnected");
  };

  ws.addEventListener("message", (event: MessageEvent) => {
    console.log("Message from server ", event.data);
  });

  const handleSubmit = (event: FormEvent) => {
    event.preventDefault();
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(inputValue);
      setInputValue("");
    } else {
      console.log("WebSocket is not open");
    }
  };

  return (
    <div>
      <form onSubmit={handleSubmit}>
        <input
          type="text"
          value={inputValue}
          onChange={(e) => setInputValue(e.target.value)}
          placeholder="Enter your message"
        />
        <button type="submit">Send</button>
      </form>
    </div>
  );
};

export default WebSocketForm;
