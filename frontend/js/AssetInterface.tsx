import React, { createContext, useContext, useEffect, useRef } from "react";
import Asset from "./Asset";
import { IncomingMessage, MessageType, OutgoingMessage } from "./Message.ts";
import { ConnectionContext, GameStateContext } from "./App";
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
  const { setConnections } = useContext(ConnectionContext);

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

      const updatedGameState = JSON.parse(
        JSON.stringify(prevGameState),
      ) as GameState;

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
    if (userInfo === undefined || setConnections === undefined) {
      return;
    }

    const connect = () => {
      const wsUrl = `ws://${window.location.host}/asset/${Asset.toString(asset)}`;
      const socket = new WebSocket(wsUrl);

      socket.onopen = () => {
        const outgoing = {
          type: MessageType.REGISTER,
          user_id: userInfo.user_id,
          username: userInfo.username,
        } as OutgoingMessage;
        socket?.send(JSON.stringify(outgoing));
      };
      socket.onclose = () => {
        console.info("WebSocket disconnected, reconnecting");
        connect();
      };
      socket.onmessage = (event: MessageEvent) => {
        const incoming = JSON.parse(event.data) as IncomingMessage;
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
            alert(incoming.error);
            break;
        }
      };
      ws.current = socket;
      setConnections((connections) => {
        return {
          ...connections,
          [asset as number]: ws,
        };
      });
    };

    connect();

    return () => {
      console.info("Closing as part of useEffect callback.");
      ws.current?.close();
    };
  }, [userInfo === undefined]);

  const place_order = (side: Side, price: number, volume: number) => {
    if (!ws.current) {
      console.error("ws.current not set, no order placed");
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
