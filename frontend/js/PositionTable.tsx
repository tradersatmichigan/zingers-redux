import React, { useContext } from "react";
import Order from "./Order";
import Side from "./Side";
import Asset from "./Asset";
import { ConnectionContext } from "./App";
import { MessageType, OutgoingMessage } from "./Message";

const PositionTable = ({ side, orders }: { side: Side; orders: Order[] }) => {
  const { connections } = useContext(ConnectionContext);

  const cancel_order = (asset: Asset, order_id: number) => {
    return () => {
      const ws = connections[asset as number];
      if (!ws.current) {
        console.error("ws.current not set, no order canceled");
        return;
      }
      const outgoing = {
        type: MessageType.CANCEL,
        order_id: order_id,
      } as OutgoingMessage;
      ws.current.send(JSON.stringify(outgoing));
    };
  };

  return (
    <>
      <p>{side === Side.BUY ? "BIDS" : "ASKS"}</p>
      <table>
        <thead>
          <tr>
            <th></th>
            <th>Asset</th>
            <th>Price</th>
            <th>Volume</th>
          </tr>
        </thead>
        <tbody>
          {orders.map((order) => (
            <tr key={order.order_id}>
              <td>
                <button
                  type="button"
                  onClick={cancel_order(order.asset, order.order_id)}
                >
                  Cancel
                </button>
              </td>
              <td>{Asset.toString(order.asset)}</td>
              <td>{order.price}</td>
              <td>{order.volume}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </>
  );
};

export default PositionTable;
