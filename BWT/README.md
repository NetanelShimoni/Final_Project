# BWT

The Burrows–Wheeler transform (BWT, also called block-sorting compression) rearranges a character string into runs of similar characters. This is useful for compression, since it tends to be easy to compress a string that has runs of repeated characters by techniques such as move-to-front transform and run-length encoding. More importantly, the transformation is reversible, without needing to store any additional data except the position of the first original character. The BWT is thus a "free" method of improving the efficiency of text compression algorithms, costing only some extra computation.

### Example

The transform is done by sorting all the circular shifts of a text in lexicographic order and by extracting the last column and the index of the original string in the set of sorted permutations of S.

Given an input string S = ^BANANA| (step 1 in the table below), rotate it N times (step 2), where N = 8 is the length of the S string considering also the symbol ^ representing the start of the string and the red | character representing the 'EOF' pointer; these rotations, or circular shifts, are then sorted lexicographically (step 3). The output of the encoding phase is the last column L = BNN^AA|A after step 3, and the index (0-based) I of the row containing the original string S, in this case I = 6. 

![תמונה](https://user-images.githubusercontent.com/57719538/130828711-e9cab7ee-9ee8-4587-b31e-513ecc6870fe.png)
 
