import React from "react";
import Asset from "./Asset";
import OrderTable from "./OrderTable";
import Side from "./Side";
import OrderForm from "./OrderForm";

const OrderBook = ({ asset }: { asset: Asset }) => {
  return (
    <>
      <p>{Asset.abreviate(asset)} orderbook</p>
      <OrderTable asset={asset} side={Side.BUY} />
      <OrderTable asset={asset} side={Side.SELL} />
      <OrderForm />
    </>
  );
};

export default OrderBook;
