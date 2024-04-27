import os


def f_bandwidth_extract(file_path):
    bandwidths = []
    with open(file_path, "r") as file:
        for line in file:
            if "%" in line:
                parts = line.split()
                if "Gbits/sec" in parts:
                    bandwidth_gbits = float(parts[6])
                    bandwidth_mbits = bandwidth_gbits * 1000
                    bandwidths.append(bandwidth_mbits)
                elif "Mbits/sec" in parts:
                    bandwidth_mbits = float(parts[6])
                    bandwidths.append(bandwidth_mbits)
    return bandwidths


def f_average_bandwidth(folder_path, time_name, prefix):
    all_bandwidths = []
    for file_name in os.listdir(folder_path):
        if time_name not in file_name:
            continue
        file_path = os.path.join(folder_path, file_name)
        if os.path.isfile(file_path):
            bandwidths = f_bandwidth_extract(file_path)
            for bandwidth in bandwidths:
                all_bandwidths.append(bandwidth)
    if len(all_bandwidths) == 0:
        print("No bandwidth data found.")
        return
    average_bandwidth = sum(all_bandwidths) / len(all_bandwidths)
    result_file_path = os.path.join(folder_path, f"{prefix}.txt")
    with open(result_file_path, "w") as file:
        file.write(f"Average Bandwidth: {average_bandwidth:.2f} Mbits/sec")
    print(f"Average Bandwidth: {average_bandwidth:.2f} Mbits/sec")


def f_print_file_contents(file_path):
    try:
        with open(file_path, "r") as file:
            for line in file:
                print(line, end="")
    except FileNotFoundError:
        print("The file was not found.")
    except IOError:
        print("An error occurred while reading the file.")
