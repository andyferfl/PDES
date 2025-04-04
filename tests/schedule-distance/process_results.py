import re
import glob
import os
import pandas as pd
import seaborn as sns
import numpy as np
import matplotlib.pyplot as plt 

RESULTS_PATH = "results/"

def delete_outliers(df, column):
    Q1 = df[column].quantile(0.25)
    Q3 = df[column].quantile(0.75)
    IQR = Q3 - Q1
    lower = Q1 - 1.5*IQR
    upper = Q3 + 1.5*IQR

    upper_array = np.where(df[column] >= upper)[0]
    lower_array = np.where(df[column] <= lower)[0]

    df.drop(index=upper_array, inplace=True)
    df.drop(index=lower_array, inplace=True)

    print("New Shape: ", df.shape)
    sns.boxplot(df[column])
    return df

def parse_pdes(file):
    file = open(file, mode = 'r')
    lines = file.readlines()
    file.close()

    start_time  = []
    end_time    = []
    total_time  = []
    kernels     = []
    kernel_id   = []
    lp_kernel   = []
    threads     = []
    executed    = []
    commited    = []
    reprocessed = []
    rollbacks   = []
    antimessages= []
    r_frequency = []
    r_length    = []
    efficiency  = []
    event_cost  = []
    ema         = []
    checkpoint  = []
    recovery    = []
    log_size    = []
    idle_cycles = []
    gvt         = []
    gvt_redux   = []
    time_speed  = []
    memory      = []
    peak_memory = []

    for line in lines:
        tmp = re.search(r"SIMULATION STARTED AT ..... : (.*)", line)
        if tmp: 
            start_time.append(tmp.group(1))
            continue

        tmp = re.search(r"SIMULATION FINISHED AT .... : (.*)", line)
        if tmp:
            end_time.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL SIMULATION TIME ..... : (.*)", line)
        if tmp: 
            total_time.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL KERNELS ............. : (.*)", line)
        if tmp: 
            kernels.append(tmp.group(1))
            continue

        tmp = re.search(r"KERNEL ID ................. : (.*)", line)
        if tmp: 
            kernel_id.append(tmp.group(1))
            continue

        tmp = re.search(r"LPs HOSTED BY KERNEL....... : (.*)", line)
        if tmp:
            lp_kernel.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL_THREADS ............. : (.*)", line)
        if tmp:
            threads.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL EXECUTED EVENTS ..... : (.*)", line)
        if tmp:
            executed.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL COMMITTED EVENTS..... : (.*)", line)
        if tmp:
            commited.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL REPROCESSED EVENTS... : (.*)", line)
        if tmp:
            reprocessed.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL ROLLBACKS EXECUTED... : (.*)", line)
        if tmp:
            rollbacks.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL ANTIMESSAGES......... : (.*)", line)
        if tmp:
            antimessages.append(tmp.group(1))
            continue

        tmp = re.search(r"ROLLBACK FREQUENCY......... : (.*)", line)
        if tmp:
            r_frequency.append(tmp.group(1))
            continue

        tmp = re.search(r"ROLLBACK LENGTH............ : (.*)", line)
        if tmp:
            r_length.append(tmp.group(1))
            continue

        tmp = re.search(r"EFFICIENCY................. : (.*)", line)
        if tmp:
            efficiency.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE EVENT COST .EMA.... : (.*)", line)
        if tmp:
            ema.append(tmp.group(1))
            continue
        
        tmp = re.search(r"AVERAGE EVENT COST......... : (.*)", line)
        if tmp:
            event_cost.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE CHECKPOINT COST.... : (.*)", line)
        if tmp:
            checkpoint.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE RECOVERY COST...... : (.*)", line)
        if tmp:
            recovery.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE LOG SIZE........... : (.*)", line)
        if tmp:
            log_size.append(tmp.group(1))
            continue

        tmp = re.search(r"IDLE CYCLES................ : (.*)", line)
        if tmp:
            idle_cycles.append(tmp.group(1))
            continue

        tmp = re.search(r"LAST COMMITTED GVT ........ : (.*)", line)
        if tmp:
            gvt.append(tmp.group(1))
            continue

        tmp = re.search(r"NUMBER OF GVT REDUCTIONS... : (.*)", line)
        if tmp:
            gvt_redux.append(tmp.group(1))
            continue

        tmp = re.search(r"SIMULATION TIME SPEED...... : (.*)", line)
        if tmp:
            time_speed.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE MEMORY USAGE....... : (.*)", line)
        if tmp:
            memory.append(tmp.group(1))
            continue

        tmp = re.search(r"PEAK MEMORY USAGE.......... : (.*)", line)
        if tmp:
            peak_memory.append(tmp.group(1))
            continue 


    
    df = pd.DataFrame({
        "Start time"                : start_time,
        "Finish time"               : end_time,
        "Total time"                : total_time,
        "Total kernels"             : kernels,
        "Kernel id"                 : kernel_id,
        "LPs"                       : lp_kernel,
        "Total threads"             : threads,
        "Total executed events"     : executed,
        "Total commited events"     : commited,
        "Total reprocessed events"  : reprocessed,
        "Total rollbacks"           : rollbacks,
        "Total antimessages"        : antimessages,
        "Rollback frequency"        : r_frequency,
        "Rollback length"           : r_length,
        "Efficiency"                : efficiency,
        "Average event cost"        : event_cost,
        "Average event cost (EMA)"  : ema,
        "Average checkpoint cost"   : checkpoint,
        "Average recovery cost"     : recovery,
        "Average log size"          : log_size,
        "Idle cycles"               : idle_cycles,
        "Last commited gvt"         : gvt,
        "Number of gvt reductions"  : gvt_redux,
        "Simulation time speed"     : time_speed,
        "Average memory usage"      : memory,
        "Peak memory usage"         : peak_memory
    })
        
    cols = ["Total kernels","Kernel id","LPs","Total threads","Total executed events","Total commited events","Total reprocessed events","Idle cycles","Last commited gvt","Number of gvt reductions"]
    df[cols] = df[cols].apply(pd.to_numeric)
    #df['Total time'] = df['Total time'].str.replace('seconds', '')
    #df['Total time'] = df['Total time'].apply(pd.to_numeric)
    
    #df = delete_outliers(df,'Total commited events')
    
    return df

def parse_pdes_wr(file_name):
    file = open(file_name, mode = 'r')
    lines = file.readlines()
    file.close()

    total_time  = []
    commited    = []


    for line in lines:

        tmp = re.search(r"committed (.*) events in (.*),", line)
        if tmp: 
            total_time.append(tmp.group(2))
            commited.append(tmp.group(1))
            continue



    
    df_wr = pd.DataFrame({
        "Total time"                : total_time,
        "Total commited events"     : commited
    })
        
    print (df_wr)
    
    df_wr['Total time'] = df_wr['Total time'].str.replace('s', '')
    df_wr['Total time'] = df_wr['Total time'].apply(pd.to_numeric)
    
    df_wr.info(verbose=True)
    cols = ["Total time","Total commited events"]
    df_wr[cols] = df_wr[cols].apply(pd.to_numeric)
    #df_wr = delete_outliers(df_wr,'Total commited events')
    
    return df_wr

def parse_des(file_path):
    
    file = open(file_path, mode = 'r')
    lines = file.readlines()
    file.close()

    start_time  = []
    end_time    = []
    total_time  = []
    lp_kernel   = []
    executed    = []
    event_cost  = []
    ema         = []
    gvt         = []
    time_speed  = []
    memory      = []
    peak_memory = []

    for line in lines:
        tmp = re.search(r"SIMULATION STARTED AT ..... : (.*)", line)
        if tmp: 
            start_time.append(tmp.group(1))
            continue

        tmp = re.search(r"SIMULATION FINISHED AT .... : (.*)", line)
        if tmp:
            end_time.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL SIMULATION TIME ..... : (.*)", line)
        if tmp: 
            total_time.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL LPs.................. : (.*)", line)
        if tmp:
            lp_kernel.append(tmp.group(1))
            continue

        tmp = re.search(r"TOTAL EXECUTED EVENTS ..... : (.*)", line)
        if tmp:
            executed.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE EVENT COST .EMA.... : (.*)", line)
        if tmp:
            ema.append(tmp.group(1))
            continue
        
        tmp = re.search(r"AVERAGE EVENT COST......... : (.*)", line)
        if tmp:
            event_cost.append(tmp.group(1))
            continue

        tmp = re.search(r"LAST COMMITTED GVT ........ : (.*)", line)
        if tmp:
            gvt.append(tmp.group(1))
            continue

        tmp = re.search(r"SIMULATION TIME SPEED...... : (.*)", line)
        if tmp:
            time_speed.append(tmp.group(1))
            continue

        tmp = re.search(r"AVERAGE MEMORY USAGE....... : (.*)", line)
        if tmp:
            memory.append(tmp.group(1))
            continue

        tmp = re.search(r"PEAK MEMORY USAGE.......... : (.*)", line)
        if tmp:
            peak_memory.append(tmp.group(1))
            continue 

    
    df_des = pd.DataFrame({
        "Start time"                : start_time,
        "Finish time"               : end_time,
        "Total time"                : total_time,
        "LPs"                       : lp_kernel,
        "Total commited events"     : executed,
        "Average event cost"        : event_cost,
        "Average event cost (EMA)"  : ema,
        "Last commited gvt"         : gvt,
        "Simulation time speed"     : time_speed,
        "Average memory usage"      : memory,
        "Peak memory usage"         : peak_memory
    })
        

    cols = ["LPs","Total commited events","Last commited gvt"]
    df_des[cols] = df_des[cols].apply(pd.to_numeric)
    df_des['Total time'] = df_des['Total time'].str.replace('seconds', '')
    df_des['Total time'] = df_des['Total time'].apply(pd.to_numeric)

    #df_des = delete_outliers(df_des,'Total executed events')
    #df_des = delete_outliers(df_des,'Total time')
    #df_des[cols].describe()

    return df_des

def parse_sst(file_name):
    with open(file_name, 'r') as file:
        lines = file.readlines()

    total_time = []
    commited = []

    for line in lines:
        tmp = re.search(r"Total time:\s+([\d\.]+)\s+seconds", line)
        if tmp:
            total_time.append(tmp.group(1))
            continue

        tmp = re.search(r"Grand total sends:\s+\d+, receives:\s+(\d+)", line)
        if tmp:
            commited.append(tmp.group(1))
            continue

    df_sst = pd.DataFrame({
        "Total time": total_time,
        "Total commited events": commited
    })

    df_sst['Total time'] = df_sst['Total time'].apply(pd.to_numeric)
    df_sst['Total commited events'] = df_sst['Total commited events'].apply(pd.to_numeric)

    return df_sst

tw_data = None

for file_path in glob.glob(RESULTS_PATH+'lookahead_*_timewarp_*_wt_*_lp.o'):
    filename = os.path.basename(file_path)
    parts = filename.split('_')

    if len(parts) < 7:
        continue

    lookahead = parts[1]
    threads = parts[3]
    lp = parts[5]
    algorithm = 'Time Warp'

    metrics = parse_pdes(file_path)
    
    metrics['lookahead'] = lookahead
    metrics['algorithm'] = algorithm

    tw_data = pd.concat([tw_data, metrics])

wr_data = None

for file_path in glob.glob(RESULTS_PATH+'lookahead_*_windowracer_*_wt_*_lp.o'):
    filename = os.path.basename(file_path)
    parts = filename.split('_')

    if len(parts) < 7:
        continue

    lookahead = parts[1]
    threads = parts[3]
    lp = parts[5]
    algorithm = 'Window Racer'

    metrics = parse_pdes_wr(file_path)
    
    metrics['lookahead'] = lookahead
    metrics['algorithm'] = algorithm
    metrics['LPs'] = lp
    metrics['Total threads'] = threads

    wr_data = pd.concat([wr_data, metrics])

des_data = None

for file_path in glob.glob(RESULTS_PATH+'lookahead_*_seq_*_lp.o'):
    filename = os.path.basename(file_path)
    parts = filename.split('_')

    lookahead = parts[1]
    lp = parts[3]

    metrics = parse_des(file_path)

    metrics['lookahead'] = lookahead
    metrics['algorithm'] = 'sequential'
    metrics['Total threads'] = 0

    des_data = pd.concat([des_data, metrics])


sst_data = None

for file_path in glob.glob(RESULTS_PATH + 'lookahead_*_sst_*_wt_*_lp.o'):
    filename = os.path.basename(file_path)
    parts = filename.split('_')

    if len(parts) < 7:
        continue

    lookahead = parts[1]
    threads = parts[3]
    lp = parts[5]
    algorithm = 'SST'

    metrics = parse_sst(file_path)

    metrics['lookahead'] = lookahead
    metrics['algorithm'] = algorithm
    metrics['LPs'] = lp
    metrics['Total threads'] = threads

    sst_data = pd.concat([sst_data, metrics])

tw_data = tw_data[tw_data['Total time'].notna()]
tw_data = tw_data[tw_data['Total commited events'].notna()]

wr_data = wr_data[wr_data['Total time'].notna()]
wr_data = wr_data[wr_data['Total commited events'].notna()]

des_data = des_data[des_data['Total time'].notna()]
des_data = des_data[des_data['Total commited events'].notna()]

sst_data = sst_data[sst_data['Total time'].notna()]
sst_data = sst_data[sst_data['Total commited events'].notna()]

tw_data['Total time'] = tw_data['Total time'].str.replace('seconds', '')
tw_data['Total time'] = tw_data['Total time'].apply(pd.to_numeric)

sst_data['Total time'] = sst_data['Total time'].apply(pd.to_numeric)

columns = ['Total time', 'LPs', 'Total commited events', 'lookahead', 'algorithm', 'Total threads']

filtered_tw = tw_data[columns]
filtered_wr = wr_data[columns]
filtered_des = des_data[columns]
filtered_sst = sst_data[columns]

union_df = pd.concat([filtered_des, filtered_tw, filtered_wr, filtered_sst], ignore_index=True)
print(union_df)

grouped_mean = union_df.groupby(['lookahead', 'algorithm', 'Total threads','LPs']).mean()
grouped_mean['Events per second'] = grouped_mean.apply(lambda row: row['Total commited events'] / (row['Total time'] if row['Total time'] else 0.000001), axis=1)
print(grouped_mean)
grouped_mean.sort_values(by='Events per second', ascending=False)
grouped_mean.to_csv('output.csv')



