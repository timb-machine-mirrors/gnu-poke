#!/usr/bin/python3


if __name__ == '__main__':
    import argparse

    PARSER = argparse.ArgumentParser(
        "Generate a AFL compatible dictionary for poke")
    PARSER.add_argument("bison_file", nargs=1, type=str)
    ARGS = PARSER.parse_args()

    with open(ARGS.bison_file[0], "r") as bison_file:
        for line in bison_file:
            if line[0] != '%':
                continue

            entries = line.split()

            if entries[0] == '%start':
                break

            if entries[0] in (
                    # bison instructions
                    '%define', '%locations', '%name-prefix', '%lex-param',
                    '%parse-param', '%initial-action', '%{', '%}', '%union',
                    '%destructor',
                    # not sure whether we should exclude this:
                    '%type'
            ):
                continue

            if 'START_PROGRAM;' in entries:
                continue

            start = entries[0][1:].lower() + '=" '
            for entry in entries[1:]:
                if entry[0] == '<':
                    continue

                print(start + entry.replace("'", "").lower() + ' "')

    for extra_token in ('{', '}', ',', '[', ']', ';'):
        print(f"extra_token=\" {extra_token} \"")
