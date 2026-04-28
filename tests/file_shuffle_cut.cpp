0 // Program: file_shuffle_cut.cpp
1 // Author: Chris Gill
2 // Purpose: reads lines of a text file and numbers them, shuffles them, and
3 //          cuts them into separate files with fragments of the original text
4 
5 #include <iostream>
6 #include <fstream>
7 #include <vector>
8 #include <algorithm>
9 #include <sstream>
10 using namespace std;
11 
12 // return codes for success or failure
13 const int success = 0;
14 const int wrong_number_of_arguments = -1;
15 const int input_file_open_failed = -2;
16 const int output_file_open_failed = -3;
17 
18 // constants for command line indexing
19 const int program_name_index = 0;
20 const int file_name_index = 1;
21 const int fragments_index = 2;
22 const int expected_argc = 3;
23 
24 // struct to hold a numbered line of text
25 struct numbered_line {
26     numbered_line() : number(0) {}
27     int number;
28     string text;
29 };
30 
31 // outputs proper usage syntax for the program
32 int usage (const char *program_name, int result) {
33     cout << "usage: " << program_name
34          << " <file name> <number of fragments>" << endl;
35     return result;
36 }
37 
38 // writes out a range of lines into a named output file
39 int write_fragment (vector<numbered_line>::const_iterator iter,
40                     vector<numbered_line>::const_iterator stop,
41                     const char * filename)
42 {
43     ofstream ofs (filename);
44     if (!ofs) {
45         cout << "Could not open output file " <<  filename << endl;
46         return output_file_open_failed;
47     }
48 
49     while (iter != stop) {
50         ofs << iter->number << " " << iter->text << endl;
51         iter++;
52     }
53 
54     return success;
55 }
56 
57 
58 int main (int argc, char *argv[]) {
59 
60     // check command line argument count
61     if (argc != expected_argc) {
62         // suggest how to run the program correctly
63         return usage(argv[program_name_index], wrong_number_of_arguments);
64     }
65 
66     // check ability to open input file
67     ifstream ifs (argv[file_name_index]);
68     if (!ifs) {
69         cout << "Could not open file " <<  argv[file_name_index] << endl;
70         return usage(argv[program_name_index], input_file_open_failed);
71     }
72 
73     // fill a vector with numbered lines from the file
74     vector<numbered_line> nlv;
75     numbered_line nl;
76     while (getline(ifs, nl.text)) {
77         nlv.push_back(nl);
78         nl.number++;
79     }
80 
81     // shuffle the numbered lines in the vector
82     random_shuffle (nlv.begin(), nlv.end());
83 
84     // extract (and possibly reduce) number of fragments to create
85     int fragments;
86     istringstream iss (argv[fragments_index]);
87     iss >> fragments;
88     if (fragments > nlv.size()) {
89         fragments = nlv.size();
90     }
91 
92     // calculate number of lines per fragment, initialize iterators
93     int lines_per_fragment = nlv.size() / fragments;
94     vector<numbered_line>::const_iterator start = nlv.begin();
95     vector<numbered_line>::const_iterator stop = start + lines_per_fragment;
96 
97     // output fragments of shuffled text into files
98     for (int fragment = 1; fragment <= fragments; ++fragment) {
99         if (stop > nlv.end() || fragment == fragments) stop = nlv.end();
100 
101         ostringstream file_name_stream;
102         file_name_stream << argv[file_name_index] << "_" << fragment;
103         int result =  write_fragment (start, stop,
104                                       file_name_stream.str().c_str());
105         if (result != success) return result;
106         start += lines_per_fragment;
107         stop = start + lines_per_fragment;
108     }
109 
110     // report statistics for what was done
111     cout << nlv.size() << " lines" << endl;
112     cout << fragments << " fragments" << endl;
113     cout << lines_per_fragment << " lines_per_fragment" << endl;
114 
115     return success;
116 }
