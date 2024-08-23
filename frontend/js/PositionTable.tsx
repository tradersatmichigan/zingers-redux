import React from "react";
import Order from "./Order";
import Side from "./Side";
import Asset from "./Asset";

const PositionTable = ({ side, orders }: { side: Side; orders: Order[] }) => {
  return (
    <>
      <p>{side === Side.BUY ? "BIDS" : "ASKS"}</p>
      <table>
        <thead>
          <tr>
            <th>Asset</th>
            <th>Price</th>
            <th>Volume</th>
          </tr>
        </thead>
        <tbody>
          {orders.map((order) => (
            <tr key={order.order_id}>
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
