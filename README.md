# Overview

This project includes:
- `data_loader`(solution_1/): reads `points.txt`, `categories.txt`, `groups.txt` and populates PostgreSQL.
- `region_query` (solution_2/): executes JSON-defined queries; supports `operator_crop` with options `category`, `one_of_groups` and `proper`.
- enhanced `region_query` (solution_3/): extend `region_query` with supporting `operator_and` and `operator_or`; support nested format JSON queries.

# Dependencies

``` bash
sudo apt install -y cmake g++ libpqxx-dev libpq-dev libgflags-dev nlohmann-json3-dev postgresql
```

# Build

## `data_loader`

```bash
cd solution_1
mkdir build && cd build
cmake ..
make
```

## `region_query`

```bash
cd solution_3
mkdir build && cd build
cmake ..
make
```

# Usage

## `data_loader`

```bash
./data_loader \
  --data_directory="/path/to/data/" \
  --conn="dbname=YOUR_DB user=USER password=PASSWD host=localhost" \
```

optional flags:
- `--truncate`: truncate all tables in the database before inserting data;
default: `false`

## `region_query`

```bash
./region_query \
  --query="/path/to/query/json/file/" \
  --conn="dbname=YOUR_DB user=USER password=PASSWD host=localhost" \
```

optional flags:
- `--out`: output path;
default: `result.txt`
- `--debug`(only available for enhanced one): printing SQL;
default: `false`
