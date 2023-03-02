import argparse
import os

NUM_VALS = 10


def generate_vals(seed):
    start_val = seed * NUM_VALS
    return [v for v in range(start_val, start_val+NUM_VALS)]


def produce_file(full_path, seed, vals):
    vals_bytes = bytes(vals)
    try:
        with open(full_path, "wb") as f:
            try:
                f.write(vals_bytes)
            except:
                print("File {}: Cannot write".format(seed))
    except:
        print("File {}: Cannot open".format(seed))


def main():
    parser = argparse.ArgumentParser("Produce data")
    parser.add_argument("num_files", type=int,
                        help="Number of files to transfer")
    parser.add_argument("producer_directory", type=str,
                        help="Directory where the producer will save files")
    args = parser.parse_args()
    if args.num_files <= 0:
        raise ValueError("'num_files' must be greater than 0")
    dir_path = os.path.abspath(os.path.expanduser(args.producer_directory))
    if not os.path.isdir(dir_path):
        raise FileNotFoundError("The provided directory ({}) does not exist".format(dir_path))
    for seed in range(args.num_files):
        vals = generate_vals(seed)
        full_path = os.path.join(dir_path, "data{}.txt".format(seed))
        produce_file(full_path, seed, vals)


if __name__ == "__main__":
    main()
