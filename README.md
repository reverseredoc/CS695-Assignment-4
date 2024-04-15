# CS695-Assignment-4

## Team Members 
| Name | Roll Number |
| --- | --- |
|Isha Arora | 210050070|
|Karan Godara | 210050082|

## Explanation of Submission Directory and Running Instructions

1. [Code folder](Code)
   - This folder contains the 10 guest VM files, each with a different memory access pattern. The names of the files match those in the report and indicate the memory access pattern of the guest VM.
   - Additionally, it contains skvm files:
     - [simple_kvm.c](Code\simple_kvm.c) : KVM with parameters mentioned in the memmgmt paper, with Sampling period 30 and sampling size 100.
     - [simple_kvm_SP10_SS100](Code/simple_kvm_SP10_SS100) : KVM with sampling period 10 and sampling size 100.
     - [simple_kvm_SP50_SS100](Code/simple_kvm_SP50_SS100) : KVM with sampling period 50 and sampling size 100.
     - [simple_kvm_SP30_SS50](Code/simple_kvm_SP30_SS50) : KVM with sampling period 30 and sampling size 50.
     - [simple_kvm_SP30_SS200](Code/simple_kvm_SP30_SS200) : KVM with sampling period 30 and sampling size 200.

## Running Instructions

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

**Note:** If you want a KVM with different parameters (default are the ones in the paper SP = 30 and SS = 100), rename that KVM as `simple_kvm.c` and run the above commands.

## Results Folder

[Results folder](Results)
- This folder contains the results (both graphs and result files) obtained for each KVM configuration for each memory access pattern of the guest.

## Report

[Report.pdf](Report.pdf)
- This is our report, which contains in great detail the motivation of our experiment, aim of our experiment, the experimental setup created by us, experiments designed by us, the results obtained from the experiments, observations, conclusions drawn from each experiment, and our final conclusion.
