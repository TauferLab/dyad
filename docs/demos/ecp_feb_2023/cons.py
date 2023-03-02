import argparse
import os

NUM_VALS = 10


def vals_are_valid(seed, vals):
    start_val = seed * NUM_VALS
    return all([vals[v - start_val] == v for v in range(
        start_val, start_val+NUM_VALS)])


def consume_file(full_path, seed):
    try:
        with open(full_path, "rb") as f:
            try:
                return list(f.read())
            except:
                print("File {}: Cannot read".format(seed))
    except:
        print("File {}: Cannot open".format(seed))


def main():
    parser = argparse.ArgumentParser("Consume data")
    parser.add_argument("num_files", type=int,
                        help="Number of files to transfer")
    parser.add_argument("consumer_directory", type=str,
                        help="Directory where the consumer will save files")
    args = parser.parse_args()
    if args.num_files <= 0:
        raise ValueError("'num_files' must be greater than 0")
    dir_path = os.path.abspath(os.path.expanduser(args.producer_directory))
    if not os.path.isdir(dir_path):
        raise FileNotFoundError("The provided directory ({}) does not exist".format(dir_path))
    for seed in range(args.num_files):
        full_path = os.path.join(dir_path, "data{}.txt".format(seed))
        vals = consume_file(full_path, seed)
        if vals_are_valid(seed, vals):
            print("File {}: OK".format(seed))
        else:
            print("File {}: BAD".format(seed))


if __name__ == "__main__":
    main()
