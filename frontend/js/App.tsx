import React, { useEffect, useState } from "react";
import LoginResponse from "./LoginResponse";
import Order from "./Order";
import StateResponse from "./State";

const App = () => {
  const [userId, setUserId] = useState<number>(-1);
  const [cash, setCash] = useState<number>(0);
  const [buyingPower, setBuyingPower] = useState<number>(0);
  const [orders, setOrders] = useState<{ [key: string]: Order }>({});

  useEffect(() => {
    fetch(`/api/login`, {
      method: "POST",
      credentials: "include",
      headers: {
        Username: "conner",
        Password: "password",
      },
    })
      .then((response) => {
        if (!response.ok) throw Error(response.statusText);
        return response.json() as Promise<LoginResponse>;
      })
      .then((data: LoginResponse) => {
        if (data.error) {
          console.log(data.error);
          return;
        }
        setUserId(data.user_id);
      })
      .catch((error) => console.error(error));
  }, []);

  useEffect(() => {
    fetch(`/api/state`, {
      credentials: "include",
      headers: {
        "User-Id": String(userId),
      },
    })
      .then((response) => {
        if (!response.ok) throw Error(response.statusText);
        return response.json() as Promise<StateResponse>;
      })
      .then((data: StateResponse) => {
        if (data.error) {
          console.log(data.error);
          return;
        }
        setCash(data.cash);
        setBuyingPower(data.buying_power);
        setOrders(data.orders);
      })
      .catch((error) => console.error(error));
  }, [userId]);

  return (
    <div>
      <p>cash: {cash}</p>
      <p>buyingPower: {buyingPower}</p>
      <p>orders: {JSON.stringify(orders)}</p>
      <p>userId: {userId}</p>
    </div>
  );
};

export default App;
