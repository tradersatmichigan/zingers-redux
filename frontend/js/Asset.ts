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
  export function toString(asset: Asset): string {
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
  }
}

export { int_to_asset };
export default Asset;
