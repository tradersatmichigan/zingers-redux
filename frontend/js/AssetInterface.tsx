import React, { useContext, useEffect, useRef } from "react";
import Asset from "./Asset";
import { IncomingMessage, MessageType, OutgoingMessage } from "./Message.ts";
import { GameStateContext, UserInfoContext } from "./App";

const AssetInterface = ({
  asset,
  onregister,
}: {
  asset: Asset;
  onregister: (asset: Asset) => void;
}) => {
  const userInfo = useContext(UserInfoContext);
  const { gameState, setGameState } = useContext(GameStateContext);

  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    console.log("userInfo:", userInfo);
    if (userInfo === undefined) {
      return;
    }
    const wsUrl = `ws://${window.location.host}/asset/${Asset.toString(asset)}`;
    const socket = new WebSocket(wsUrl);

    socket.onopen = () => {
      console.log("WebSocket connected");
      const outgoing = {
        type: MessageType.REGISTER,
        user_id: userInfo.user_id,
      } as OutgoingMessage;
      socket.send(JSON.stringify(outgoing));
    };

    socket.onclose = () => {
      console.log("WebSocket disconnected");
    };

    socket.onmessage = (event: MessageEvent) => {
      const message = JSON.parse(event.data) as IncomingMessage;
      switch (message.type as MessageType) {
        case MessageType.REGISTER:
          onregister(asset);
        case MessageType.ORDER:
          break;
        case MessageType.CANCEL:
          break;
        case MessageType.ERROR:
          break;
      }
      console.log(`Message from ${Asset.toString(asset)} server:`, message);
    };

    ws.current = socket;
    return () => socket.close();
  }, [userInfo === undefined]);

  return (
    <>
      <p>{asset}</p>
      <p>gameState: {JSON.stringify(gameState)}</p>
    </>
  );
};

export default AssetInterface;
