enum Asset {
  DRESSING = 0,
  RYE = 1,
  SWISS = 2,
  PASTRAMI = 3,
}

const int_to_asset: Asset[] = [
  Asset.DRESSING,
  Asset.RYE,
  Asset.SWISS,
  Asset.PASTRAMI,
];

namespace Asset {
  export const toString = (asset: Asset) => {
    switch (asset) {
      case Asset.DRESSING:
        return "dressing";
      case Asset.RYE:
        return "rye";
      case Asset.SWISS:
        return "swiss";
      case Asset.PASTRAMI:
        return "pastrami";
    }
  };

  export const abreviate = (asset: Asset) => {
    switch (asset) {
      case Asset.DRESSING:
        return "DRS";
      case Asset.RYE:
        return "RYE";
      case Asset.SWISS:
        return "SWS";
      case Asset.PASTRAMI:
        return "PAS";
    }
  };
}

export { int_to_asset };
export default Asset;
