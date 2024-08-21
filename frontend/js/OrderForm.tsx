import React, { useContext, useState } from "react";
import Side from "./Side";
import { PlaceOrderContext } from "./AssetInterface";

interface FormData {
  price: number;
  volume: number;
}

const OrderForm = ({ side }: { side: Side }) => {
  const place_order = useContext(PlaceOrderContext);
  const [formData, setFormData] = useState<FormData>({ price: 0, volume: 0 });

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    setFormData((prevData) => ({
      ...prevData,
      [name]: parseInt(value, 10) || 0,
    }));
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (place_order) {
      console.log("Form data submitted:", formData);
      place_order(side, formData.price, formData.volume);
    }
  };

  return (
    <form onSubmit={handleSubmit}>
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
