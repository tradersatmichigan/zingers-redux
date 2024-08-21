import React, { useContext, useEffect } from "react";
import Asset from "./Asset";
import { IncomingMessage, MessageType, OutgoingMessage } from "./Message.ts";
import { GameStateContext } from "./App";
import Side from "./Side.ts";
import Trade from "./Trade.ts";
import GameState from "./GameState.ts";
import UserInfo from "./UserInfo.ts";

const AssetInterface = ({
  asset,
  userInfo,
  handle_register_message,
}: {
  asset: Asset;
  userInfo: UserInfo | undefined;
  handle_register_message: (asset: Asset) => void;
}) => {
  const { setGameState } = useContext(GameStateContext);

  const settle_trades = (prevGameState: GameState, trades: Trade[]) => {
    for (const trade of trades) {
      const order = prevGameState.orders[trade.order_id];
      if (trade.buyer_id === userInfo?.user_id) {
        prevGameState.assets_held[asset as number] += trade.volume;
        prevGameState.selling_power[asset as number] += trade.volume;
        prevGameState.cash -= trade.price * trade.volume;
        if (order.user_id !== userInfo?.user_id) {
          prevGameState.buying_power -= trade.price * trade.volume;
        }
      }
      if (trade.seller_id === userInfo?.user_id) {
        prevGameState.assets_held[asset as number] -= trade.volume;
        if (order.user_id !== userInfo?.user_id) {
          prevGameState.selling_power[asset as number] += trade.volume;
        }
        prevGameState.cash -= trade.price * trade.volume;
        prevGameState.buying_power += trade.price * trade.volume;
      }
      if (order.volume === trade.volume) {
        delete prevGameState.orders[order.order_id];
      } else {
        prevGameState.orders[order.order_id].volume -= trade.volume;
      }
    }
  };

  const handle_order_message = (incoming: IncomingMessage) => {
    if (!setGameState) {
      console.error("setGameState:", setGameState);
      return;
    }
    setGameState((prevGameState) => {
      console.log("before:", prevGameState);
      if (!prevGameState) {
        console.error("Previous gameState is undefined");
        return prevGameState;
      }
      const updatedGameState = { ...prevGameState };
      if (incoming.trades) {
        settle_trades(updatedGameState, incoming.trades);
      }
      if (incoming.unmatched_order) {
        const order = incoming.unmatched_order;
        updatedGameState.orders[order.order_id] = order;
        switch (order.side) {
          case Side.BUY:
            updatedGameState.buying_power -= order.price * order.volume;
            break;
          case Side.SELL:
            updatedGameState.selling_power[asset as number] -= order.volume;
            break;
        }
      }
      return updatedGameState;
    });
  };

  const handle_cancel_message = (incoming: IncomingMessage) => {
    if (!setGameState) {
      console.error("setGameState:", setGameState);
      return;
    }
    setGameState((prevGameState) => {
      if (!prevGameState) {
        console.error("Previous gameState is undefined");
        return prevGameState;
      }

      if (!incoming.order_id) {
        console.error(
          "Order id must be specified for cancellations:",
          incoming,
        );
        return prevGameState;
      }

      const order = prevGameState.orders[incoming.order_id];
      if (!order) {
        console.error("No matching order id found:", incoming);
        return prevGameState;
      }

      const updatedGameState = { ...prevGameState };

      if (order.user_id === userInfo?.user_id) {
        switch (order.side) {
          case Side.BUY:
            updatedGameState.buying_power += order.price * order.volume;
            break;
          case Side.SELL:
            updatedGameState.selling_power = [...prevGameState.selling_power];
            updatedGameState.selling_power[asset as number] += order.volume;
            break;
        }
      }

      const { [order.order_id]: _, ...new_orders } = prevGameState.orders;
      updatedGameState.orders = new_orders;

      return updatedGameState;
    });
  };

  useEffect(() => {
    if (userInfo === undefined) {
      return;
    }
    const wsUrl = `ws://${window.location.host}/asset/${Asset.toString(asset)}`;
    const socket = new WebSocket(wsUrl);

    socket.onopen = () => {
      const outgoing = {
        type: MessageType.REGISTER,
        user_id: userInfo.user_id,
      } as OutgoingMessage;
      socket.send(JSON.stringify(outgoing));
    };

    socket.onclose = () => {
      console.error("WebSocket disconnected");
    };

    socket.onmessage = (event: MessageEvent) => {
      const incoming = JSON.parse(event.data) as IncomingMessage;
      console.log(`Message from ${Asset.toString(asset)} server:`, incoming);
      switch (incoming.type as MessageType) {
        case MessageType.REGISTER:
          handle_register_message(asset);
          break;
        case MessageType.ORDER:
          handle_order_message(incoming);
          break;
        case MessageType.CANCEL:
          handle_cancel_message(incoming);
          break;
        case MessageType.ERROR:
          console.error(incoming.error);
          break;
      }
    };

    return () => socket.close();
  }, [userInfo]);

  return (
    <>
      <p>{asset}</p>
    </>
  );
};

export default AssetInterface;
