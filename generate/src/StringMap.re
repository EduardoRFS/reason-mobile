include Map.Make(String);

let to_yojson = (f, map) =>
  map |> bindings |> [%to_yojson: list((string, 'a))](f);
let of_yojson = (f, json) =>
  json
  |> [%of_yojson: list((string, 'a))](f)
  |> Result.map(entries => entries |> List.to_seq |> of_seq);
