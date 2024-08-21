import React, { useContext } from "react";
import Asset from "./Asset";
import Side from "./Side";
import { GameStateContext } from "./App";

const OrderTable = ({ asset, side }: { asset: Asset; side: Side }) => {
  const { gameState } = useContext(GameStateContext);
  const consolidatedOrders: { [price: number]: number } = {};

  if (gameState) {
    Object.values(gameState.orders)
      .filter((order) => order.asset === asset && order.side === side)
      .forEach((order) => {
        if (consolidatedOrders[order.price]) {
          consolidatedOrders[order.price] += order.volume;
        } else {
          consolidatedOrders[order.price] = order.volume;
        }
      });
  }

  const orderEntries = Object.entries(consolidatedOrders).map(
    ([price, volume]) => ({
      price: Number(price),
      volume: Number(volume),
    }),
  );

  orderEntries.sort((a, b) => {
    return side === Side.BUY ? b.price - a.price : a.price - b.price;
  });

  return (
    <>
      <p>{side === Side.BUY ? "BIDS" : "ASKS"}</p>
      <table>
        <thead>
          <tr>
            <th>Price</th>
            <th>Volume</th>
          </tr>
        </thead>
        <tbody>
          {orderEntries.map((order) => (
            <tr key={order.price}>
              <td>{order.price}</td>
              <td>{order.volume}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </>
  );
};

export default OrderTable;
