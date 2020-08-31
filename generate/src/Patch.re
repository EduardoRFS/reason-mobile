open Helper;

module StringMap = {
  include Map.Make(String);

  let to_yojson = (f, map) =>
    map |> bindings |> [%to_yojson: list((string, 'a))](f);
  let of_yojson = (f, json) =>
    json
    |> [%of_yojson: list((string, 'a))](f)
    |> Result.map(entries => entries |> List.to_seq |> of_seq);
};

[@deriving yojson]
type manifest = {
  source: [@default None] option(string),
  build: [@default None] option(list(list(string))),
  install: [@default None] option(list(list(string))),
  build_env: [@default None] option(StringMap.t(string)),
  exported_env: [@default None] option(StringMap.t(string)),
  dependencies: [@default None] option(StringMap.t(string)),
  raw_dependencies: [@default None] option(StringMap.t(string)),
};

type t = {
  manifest,
  checksum: option(string),
  files_folder: option(string),
};

let root_folder = Filename.concat(Filename.current_dir_name, "patches");
let folders = Sys.readdir(root_folder) |> Array.to_list;

// TODO: think a little bit better if using .none as early return is the best idea
let get_patch_folder = (name, version) => {
  let name_and_version = name ++ "." ++ version;
  let.some name = {
    // TODO: read the version from the original manifest
    let.none () = folders |> List.find_opt((==)(name_and_version));
    folders |> List.find_opt((==)(name));
  };
  Filename.concat(root_folder, name) |> some;
};

let get_path = (name, version) => {
  switch (get_patch_folder(name, version)) {
  | Some(folder) =>
    let files_folder = Filename.concat(folder, "files");

    let.await checksum = {
      let.await files_folder_exists = Lwt_unix.file_exists(files_folder);
      files_folder_exists
        ? Lib.folder_sha1(files_folder) |> Lwt.map(some) : await(none);
    };

    let.await manifest = {
      let.await data =
        Filename.concat(folder, "package.json") |> Lib.read_file;
      Yojson.Safe.from_string(data)
      |> manifest_of_yojson
      |> Result.get_ok
      |> await;
    };

    {manifest, checksum, files_folder: Some(files_folder)} |> some |> await;
  | None => await(none)
  };
};
