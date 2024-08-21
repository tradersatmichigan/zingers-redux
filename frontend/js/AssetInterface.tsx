import React, { createContext, useContext, useEffect, useRef } from "react";
import Asset from "./Asset";
import { IncomingMessage, MessageType, OutgoingMessage } from "./Message.ts";
import { GameStateContext } from "./App";
import Side from "./Side.ts";
import Trade from "./Trade.ts";
import GameState from "./GameState.ts";
import UserInfo from "./UserInfo.ts";
import OrderBook from "./OrderBook.tsx";

const PlaceOrderContext = createContext<
  ((side: Side, price: number, volume: number) => void) | undefined
>(undefined);

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
      if (!order) {
        continue;
      }
      if (
        trade.buyer_id === trade.seller_id &&
        trade.buyer_id === userInfo?.user_id
      ) {
        switch (order.side) {
          case Side.BUY:
            prevGameState.buying_power += order.price * order.volume;
            break;
          case Side.SELL:
            prevGameState.selling_power[asset as number] += trade.volume;
            break;
        }
      } else if (trade.buyer_id === userInfo?.user_id) {
        prevGameState.assets_held[asset as number] += trade.volume;
        prevGameState.selling_power[asset as number] += trade.volume;
        prevGameState.cash -= trade.price * trade.volume;
        if (order.user_id !== userInfo?.user_id) {
          prevGameState.buying_power -= trade.price * trade.volume;
        }
      } else if (trade.seller_id === userInfo?.user_id) {
        prevGameState.assets_held[asset as number] -= trade.volume;
        if (order.user_id !== userInfo?.user_id) {
          prevGameState.selling_power[asset as number] -= trade.volume;
        }
        prevGameState.cash += trade.price * trade.volume;
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
      if (!prevGameState) {
        console.error("Previous gameState is undefined");
        return prevGameState;
      }

      // Hack to ensure we aren't modifying prevGameState directly
      const updatedGameState = JSON.parse(
        JSON.stringify(prevGameState),
      ) as GameState;

      if (incoming.trades) {
        settle_trades(updatedGameState, incoming.trades);
      }
      console.log("after settling trades:", updatedGameState);

      if (incoming.unmatched_order) {
        const order = incoming.unmatched_order;
        updatedGameState.orders[order.order_id] = order;
        if (order.user_id === userInfo?.user_id) {
          switch (order.side) {
            case Side.BUY:
              updatedGameState.buying_power -= order.price * order.volume;
              break;
            case Side.SELL:
              updatedGameState.selling_power[asset as number] -= order.volume;
              break;
          }
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
      updatedGameState.selling_power = [...prevGameState.selling_power];
      updatedGameState.assets_held = [...prevGameState.assets_held];

      if (order.user_id === userInfo?.user_id) {
        switch (order.side) {
          case Side.BUY:
            updatedGameState.buying_power += order.price * order.volume;
            break;
          case Side.SELL:
            updatedGameState.selling_power[asset as number] += order.volume;
            break;
        }
      }

      const { [order.order_id]: _, ...new_orders } = prevGameState.orders;
      updatedGameState.orders = new_orders;

      return updatedGameState;
    });
  };

  const ws = useRef<WebSocket | undefined>(undefined);

  useEffect(() => {
    if (userInfo === undefined) {
      return;
    }

    const onopen = () => {
      const outgoing = {
        type: MessageType.REGISTER,
        user_id: userInfo.user_id,
      } as OutgoingMessage;
      ws.current?.send(JSON.stringify(outgoing));
    };

    const onclose = () => {
      console.error("WebSocket disconnected");
    };

    const onmessage = (event: MessageEvent) => {
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

    const wsUrl = `ws://${window.location.host}/asset/${Asset.toString(asset)}`;
    ws.current = new WebSocket(wsUrl);

    ws.current.onopen = onopen;
    ws.current.onclose = onclose;
    ws.current.onmessage = onmessage;

    const socket = ws.current;

    return () => {
      console.log("Closing as part of useEffect callback.");
      socket.close();
    };
  }, [userInfo]);

  const place_order = (side: Side, price: number, volume: number) => {
    if (!ws.current) {
      console.error("ws.current not set");
      return;
    }
    const outgoing = {
      type: MessageType.ORDER,
      asset: asset,
      side: side,
      price: price,
      volume: volume,
    } as OutgoingMessage;
    ws.current.send(JSON.stringify(outgoing));
  };

  return (
    <PlaceOrderContext.Provider value={place_order}>
      <OrderBook asset={asset} />
    </PlaceOrderContext.Provider>
  );
};

export { PlaceOrderContext };
export default AssetInterface;