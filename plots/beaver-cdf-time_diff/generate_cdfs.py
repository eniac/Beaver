import pandas as pd
import numpy as np

def generate_cdf_values(filename):
    # Load the data from the file
    data = pd.read_csv(filename, sep=" ", header=None)
    # Count the number of values greater than 1000000 in the second column
    count_greater_than_1000000 = len(data[data[1] > 500000])
    print("Number of values greater than 1000000 in the second column:", count_greater_than_1000000)
    # Filter out the rows where the second column is greater than 1000000
    filtered_data = data[data[1] <= 500000]
    filtered_data.to_csv(filename+"filtered", sep=" ", header=None, index=False)


    # Function to calculate CDF values for a given series
    # def cdf(series):
    #     sorted_series = np.sort(series)
    #     cdf = np.arange(1, len(series) + 1) / len(series)
    #     return sorted_series, cdf
    # # Calculating CDF for each distribution
    # cdf_2, cdf_values_2 = cdf(filtered_data[1])
    # cdf_3, cdf_values_3 = cdf(filtered_data[2])
    # cdf_diff, cdf_values_diff = cdf(filtered_data[1] - filtered_data[2])
    # # Printing percentile values for each distribution
    # print("\nCDF Values for Distribution in Column 2 (p1, p2, ..., p100):")
    # for i in range(1, 101):
    #     c1 = np.percentile(cdf_2, i)
    #     c2 = np.percentile(cdf_3, i)
    #     c3 = np.percentile(cdf_diff, i)
    #     print("p{0} {1} {2} {3}".format(i, c1, c2, c3))
# Use the appropriate file name
name = "65536_16_10000000.txt"
generate_cdf_values(name)