import os
import struct


# Directory which contains memory dumps.
#     Should only contain memory dump files.
#     Each memory dump file should be prefixed with "<counter variable value>_".
directory = "./music_volume"

offset = 0
data_width = 4

def decreasingOrEqual(theValues):
    for i in range(1, len(theValues)):
        if theValues[i] > theValues[i-1]:
            return False
        if (i > 1):
            if theValues[i] == theValues[i-1]:
                if theValues[i-1] >= theValues[i-2]:
                    return False
    return True

def decreasing(theValues):
    for i in range(1, len(theValues)):
        if theValues[i] >= theValues[i-1]:
            return False
    return True

def increasingOrEqual(theValues):
    for i in range(1, len(theValues)):
        if theValues[i] < theValues[i-1]:
            return False
        if (i > 1):
            if theValues[i] == theValues[i-1]:
                if theValues[i-1] <= theValues[i-2]:
                    return False
    return True

def increasing(theValues):
    for i in range(1, len(theValues)):
        if theValues[i] <= theValues[i-1]:
            return False
    return True

# Get the list of filenames, from specified directory, and put into (alpha)numeric descending order.
ascending_filenames = [f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]
filenames = list(reversed(ascending_filenames))

# Parse expected value, which to search for within each file, from name of file.
search_values = tuple([int(filename.split("_")[0]) for filename in filenames])

print('order in which files will be searched for descending values.')
print(search_values)

format_char = 'L' if data_width == 4 else 'H' if data_width == 2 else 'B'

# After reading files, files_contents will be list of lists.
fileS_contents = []

# For each filename.
for filename in filenames:
    # Open file for binary read
    with open(os.path.join(directory,filename), "rb") as file:
        # Read file and add as list within the list of lists
        file_contents = file.read()
        tmp_file_contents = struct.unpack('>' + (format_char * ((len(file_contents)-offset) // data_width)), file_contents[offset:])
        fileS_contents += [tmp_file_contents]
#test


# Get addresses, where search values match for each file
addresses = [hex(i*data_width) for i, mahtuple in enumerate(zip(*fileS_contents)) if decreasing(mahtuple)]

#print(indices)
print(addresses)

