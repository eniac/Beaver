import pandas as pd
import numpy as np

def generate_cdf_values(filename):
    # Load the data from the file
    data = pd.read_csv(filename, sep=" ", header=None)

    # Function to calculate CDF values for a given series
    def cdf(series):
        sorted_series = np.sort(series)
        cdf = np.arange(1, len(series) + 1) / len(series)
        return sorted_series, cdf

    # Calculating CDF for each distribution
    cdf_2, cdf_values_2 = cdf(data[0])

    # Printing percentile values for each distribution
    print("\nCDF Values for Distribution in Column 2 (p1, p2, ..., p100):")
    for i in range(0, 101):
        c1 = np.percentile(cdf_2, i)
        # print("p{0} {1}".format(i, c1))
        print("{0} {1}".format(c1, i/100.0))

# name = "t5_t0_dc.txt"
# name = "t5_t0_normal.txt"
name = "t5_t0_worst.txt"
generate_cdf_values(name)
