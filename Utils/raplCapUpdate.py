import re, sys


# Iterate over the input files
for input_file in sys.argv[4:]:

    with open(input_file, 'r') as f:
        data = f.read()

    data = re.sub(r'#define WHATTSCAP .*', f'#define WHATTSCAP {int(sys.argv[1])}', data)
    data = re.sub(r'#define USE_PERF .*', f'#define USE_PERF {int(sys.argv[2])}', data)
    data = re.sub(r'#define MEASUREMENTS_FILE .*', f'#define MEASUREMENTS_FILE "{sys.argv[3]}"', data)
    # Write the updated file
    with open(input_file, 'w') as f:
        f.write(data)
