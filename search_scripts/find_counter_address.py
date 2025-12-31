import os

# Directory which contains memory dumps.
#     Should only contain memory dump files.
#     Each memory dump file should be prefixed with "<counter variable value>_".
directory = "./levels"

# Get the list of filenames, from specified directory.
filenames = [f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]

# Parse expected value, which to search for within each file, from name of file.
search_values = tuple([int(filename.split("_")[0]) for filename in filenames])
print('search values')
print(search_values)

# After reading files, files_contents will be list of lists.
files_contents = []

# For each filename.
for filename in filenames:
    # Open file for binary read
    with open(os.path.join(directory,filename), "rb") as file:
        # Read file and add as list within the list of lists
        file_contents = file.read()
        files_contents += [file_contents]

# Get indices, where search values match for each file
indices = [i for i, mahtuple in enumerate(zip(*files_contents)) if mahtuple == search_values]

print(indices)
for indice in indices:
	print(hex(indice))
