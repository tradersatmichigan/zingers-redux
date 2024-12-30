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
    <table>
      <thead>
        <tr>
          <th colSpan={2}>{side === Side.BUY ? "Bids" : "Asks"}</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>Price</td>
          <td>Volume</td>
        </tr>
        {orderEntries.length > 0 ? (
          orderEntries.map((order) => (
            <tr key={order.price}>
              <td>${order.price}</td>
              <td>{order.volume}</td>
            </tr>
          ))
        ) : (
          <tr>
            <td colSpan={2}>No {side === Side.BUY ? "bids" : "asks"}</td>
          </tr>
        )}
      </tbody>
    </table>
  );
};

export default OrderTable;
