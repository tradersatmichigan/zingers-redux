import React, { useContext, useState } from "react";
import Side from "./Side";
import { PlaceOrderContext } from "./AssetInterface";

interface FormData {
  side: Side;
  price: number;
  volume: number;
}

const OrderForm = () => {
  const place_order = useContext(PlaceOrderContext);
  const [formData, setFormData] = useState<FormData>({
    side: Side.BUY,
    price: 0,
    volume: 0,
  });

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;

    setFormData((prevData) => ({
      ...prevData,
      [name]:
        name === "side"
          ? (parseInt(value, 10) as Side)
          : parseInt(value, 10) || 0,
    }));
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    place_order && place_order(formData.side, formData.price, formData.volume);
  };

  return (
    <form onSubmit={handleSubmit}>
      <label>
        <input
          type="radio"
          name="side"
          value={Side.BUY}
          checked={formData.side === Side.BUY}
          onChange={handleChange}
        />
        Buy
      </label>
      <label>
        <input
          type="radio"
          name="side"
          value={Side.SELL}
          checked={formData.side === Side.SELL}
          onChange={handleChange}
        />
        Sell
      </label>
      <label htmlFor="price">Price:</label>
      <input
        type="number"
        id="price"
        name="price"
        value={formData.price}
        onChange={handleChange}
        min="1"
        max="200"
      />
      <label htmlFor="volume">Volume:</label>
      <input
        type="number"
        id="volume"
        name="volume"
        value={formData.volume}
        onChange={handleChange}
        min="1"
      />
      <button type="submit">Submit</button>
    </form>
  );
};

export default OrderForm;
