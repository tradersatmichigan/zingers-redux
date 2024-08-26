import React, { useContext } from "react";
import Asset from "./Asset";
import OrderTable from "./OrderTable";
import Side from "./Side";
import OrderForm from "./OrderForm";
import { GameStateContext } from "./App";

const OrderBook = ({ asset }: { asset: Asset }) => {
  const { gameState } = useContext(GameStateContext);

  return (
    <>
      <h3>
        {Asset.toStringProper(asset)} (${Asset.value(asset)}) &mdash;{" "}
        {gameState?.assets_held[asset as number]} held
      </h3>
      <div
        style={{
          display: "flex",
          flexWrap: "wrap",
        }}
      >
        <div style={{ width: "35%" }}>
          <OrderTable asset={asset} side={Side.BUY} />
        </div>
        <div style={{ width: "35%" }}>
          <OrderTable asset={asset} side={Side.SELL} />
        </div>
        <div style={{ width: "30%" }}>
          <OrderForm />
        </div>
      </div>
    </>
  );
};

export default OrderBook;
