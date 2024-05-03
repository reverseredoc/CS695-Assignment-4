# CS695-Assignment-4

## Team Members 
| Name | Roll Number |
| --- | --- |
|Isha Arora | 210050070|
|Karan Godara | 210050082|

## Explanation of Submission Directory and Running Instructions
## [Code folder](Code)
   - This folder contains the 10 guest VM files, each with a different memory access pattern. The names of the files match those in the report and indicate the memory access pattern of the guest VM.
   - Additionally, it contains the following KVM files:
     - [simple-kvm.c](Code/simple-kvm.c) : KVM with parameters mentioned in the memmgmt paper, with Sampling period 30 and sampling size 100.
     - [simple-kvm_SP10_SS100](Code/simple-kvm_SP10_SS100.c) : KVM with sampling period 10 and sampling size 100.
     - [simple-kvm_SP50_SS100](Code/simple-kvm_SP50_SS100.c) : KVM with sampling period 50 and sampling size 100.
     - [simple-kvm_SP30_SS50](Code/simple-kvm_SP30_SS50.c) : KVM with sampling period 30 and sampling size 50.
     - [simple-kvm_SP30_SS200](Code/simple-kvm_SP30_SS200.c) : KVM with sampling period 30 and sampling size 200.

## Running Instructions
In the **'Code'** folder, do the following

1. Make a symbolic link 'guest.c' to the guest file you want to run:

   ```ln -s guest_file_you_want_to_run guest.c```

2. Run make:

   ```make```
   
   The VM would start running, and after every sampling period, the actual and the estimated working size estimates would be printed on the terminal.
   
   **NOTE:** All guest files (except `guest_mix_rare_chunk_rare`) have an infinite while loop. To stop the experiment, press `Ctrl + C` on the keyboard. To run the experiment for a certain time period, say x seconds, use:

   ```timeout x make```

   instead of `make`.

3. Once the experiment is done, run:
   
   ```make clean```

**Note:** If you want a KVM with different parameters (default parameters are the ones in the paper SP = 30 and SS = 100), rename that KVM as `simple-kvm.c` and run the above commands.

## [Results folder](Results)
- This folder contains the results (both graphs and result files) obtained for each KVM configuration for each memory access pattern of the guest.

## [Graph_makers](Graph_makers)
- This folder contains the codes to make graphs. To use any of the code, run the code with python3, with the file containing the data of all runs as printed on the terminal as the first argyment on the terminal

## [Report](Report.pdf)
- This is our comprehensive report that meticulously outlines the motivation, objectives, experimental setup, design, results, observations, and conclusions drawn from each experiment, culminating in our final analysis and insights.


