#!/usr/bin/env python
import os
import matplotlib.pyplot as plt
import mpl_toolkits.mplot3d.axes3d as p3
import matplotlib.patheffects as pe
import numpy as np
import time
from math import *
import copy
import statistics
from pyquaternion import Quaternion
from pathlib import Path

from mpl_toolkits.mplot3d import Axes3D
import csv

import seaborn as sns
colors = sns.color_palette("Set2")

def save_to_csv(data_list, path_to_save):
    with open(path_to_save, 'w') as csv_file:
        wr = csv.writer(csv_file, quoting=csv.QUOTE_ALL)
        for data in data_list:
            wr.writerow([data])

def relative_translation(p1, p2):
    return [p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]]


def rotation_error(r1, r2):
    diff_r = Quaternion(r1.elements - r2.elements)
    return diff_r.degrees

def translation_error(p1, p2):
    return sqrt(pow(p1[0] - p2[0],2) + pow(p1[1] - p2[1],2) + pow(p1[2] - p2[2],2))

def traj_translation_error(trj1, gt_traj):
    gt_index = 0
    errors = []
    for p in trj1:
        while gt_index < len(gt_traj) and p.timestamp > gt_traj[gt_index].timestamp:
            gt_index = gt_index + 1
        
        if gt_index < len(gt_traj):
            # print('P timestamp: ', p.timestamp, '  GT timestamp: ', gt_traj[gt_index].timestamp)
            # print('Time diff : ', p.timestamp - gt_traj[gt_index].timestamp)
            errors.append(translation_error(p.position, gt_traj[gt_index].position))

    return errors

def traj_relative_translation_error(trj1, gt_traj):
    gt_index = 0
    gt_last_index = 0
    errors = []

    last_p = None

    for p in trj1:
        while gt_index < len(gt_traj) and p.timestamp >= gt_traj[gt_index].timestamp:
            gt_index = gt_index + 1
        
        if gt_index < len(gt_traj):
            # print('P timestamp: ', p.timestamp, '  GT timestamp: ', gt_traj[gt_index].timestamp)
            # print('Time diff : ', p.timestamp - gt_traj[gt_index].timestamp)
            if last_p != None:
                relative_trans = relative_translation(p.position,last_p.position)
                relative_trans_gt = relative_translation(gt_traj[gt_index].position, gt_traj[gt_last_index].position)

                errors.append(translation_error(relative_trans, relative_trans_gt))

            last_p = p
            gt_last_index = gt_index

    return errors

def traj_relative_orientation_error(trj1, gt_traj):
    gt_index = 0
    gt_last_index = 0
    errors = []

    last_p = None

    for p in trj1:
        while gt_index < len(gt_traj) and p.timestamp >= gt_traj[gt_index].timestamp:
            gt_index = gt_index + 1
        
        if gt_index < len(gt_traj):
            # print('P timestamp: ', p.timestamp, '  GT timestamp: ', gt_traj[gt_index].timestamp)
            # print('Time diff : ', p.timestamp - gt_traj[gt_index].timestamp)
            if last_p != None:
                e = rotation_error(p.orientation, gt_traj[gt_index].orientation)

            last_p = p
            gt_last_index = gt_index

    return errors

def translation(p, t):
    return np.array([p[0]-t[0], p[1]-t[1], p[2]-t[2]])

def distance(p1, p2):
    return sqrt(pow(p1[0] - p2[0], 2) + pow(p1[1] - p2[1], 2))

def error_log(errors):
    import statistics
    # print("    Means        Max        min")
    print('Mean : ', statistics.mean(errors), '    std dev  :  ',  statistics.stdev(errors), '    Max  :  ',  max(errors), '    Min  :  ',  min(errors))


class loc_point:
    def __init__(self):
        self.timestamp = 0
        self.position = None
        self.orientation = None

# f, ax = plt.subplots()
# f.set_size_inches(6.0, 5.0)


#######################
###Load Ground Truth###
#######################
gt_path = './data/ground_truth.csv'

offset_flag = True
offset_p = [] #np.array()
offset_r = Quaternion()
offset_point = None
conj_offset_r = Quaternion()
ground_truth_traj = []

gtxs = []
gtys = []
gtzs = []

with open(gt_path) as f:
    lines = f.readlines()
    for line in lines[1:]:
        content = [float(x) for x in line.split(',')]

        new_p = loc_point()

        new_p.timestamp = content[0] / (1.0 * 10e8)

        new_p.position = np.array([content[1], content[2], content[3]])
        new_p.orientation = Quaternion(content[4], content[5], content[6], content[7])

        if offset_flag:
            offset_point = copy.copy(new_p)
            conj_offset_r = offset_point.orientation.conjugate
            offset_flag = False
        
        #### Eliminate the initial offset
        new_p.position = translation(new_p.position, offset_point.position)
        new_p.position = conj_offset_r.rotate(new_p.position)

        new_p.orientation = Quaternion(new_p.orientation.elements - offset_point.orientation.elements)

        ### coordination transform
        new_p.position = np.array([new_p.position[0], new_p.position[1], -new_p.position[2]])

        ground_truth_traj.append(new_p)

        gtxs.append(new_p.position[0])
        gtys.append(new_p.position[1])
        gtzs.append(new_p.position[2])


###### Chose partial data for better visualization
gindex = int(len(gtxs) * 2.3/7)
end_timestamp = ground_truth_traj[gindex].timestamp

print("End_timestamp : ", end_timestamp)

###############################
###Load Experimental Results###
###############################
### Baseline

txt_name = "./data/baseline.txt"

xs = []
ys = []
zs = []
test_traj = []
print(txt_name)
start_time = 0
with open(txt_name) as f:
    lines = f.readlines()
    for line in lines:
        content = [float(x) for x in line.split()]

        new_p = loc_point()
        new_p.timestamp = content[0] / (1.0 * 10e8)

        if new_p.timestamp > end_timestamp:
            break

        new_p.position = np.array([content[1], content[2], -content[3]])
        new_p.orientation = Quaternion(content[4], content[5], content[6], content[7])

        test_traj.append(new_p)

        xs.append(new_p.position[0])
        ys.append(new_p.position[1])
        zs.append(new_p.position[2])

### Abnormal timings
txt_name = "./data/abnormal.txt"
axs = []
ays = []
azs = []
test_traj = []
print(txt_name)
start_time = 0
with open(txt_name) as f:
    lines = f.readlines()
    for line in lines:
        content = [float(x) for x in line.split()]

        new_p = loc_point()
        new_p.timestamp = content[0] / (1.0 * 10e8)

        if new_p.timestamp > end_timestamp:
            break

        # print("End timestamp : ", end_timestamp, "  vs   ", new_p.timestamp )

        new_p.position = np.array([content[1], content[2], -content[3]])
        new_p.orientation = Quaternion(content[4], content[5], content[6], content[7])

        test_traj.append(new_p)

        axs.append(new_p.position[0])
        ays.append(new_p.position[1])
        azs.append(new_p.position[2])


### Mitigated by DFA

txt_name = "./data/dfa.txt"
dfa_xs = []
dfa_ys = []
dfa_zs = []
test_traj = []
print(txt_name)
start_time = 0
with open(txt_name) as f:
    lines = f.readlines()
    for line in lines:
        content = [float(x) for x in line.split()]

        new_p = loc_point()
        new_p.timestamp = content[0]  / (1.0 * 10e8)

        if new_p.timestamp > end_timestamp:
            break

        new_p.position = np.array([content[1], content[2], -content[3]])
        new_p.orientation = Quaternion(content[4], content[5], content[6], content[7])

        test_traj.append(new_p)

        dfa_xs.append(new_p.position[0])
        dfa_ys.append(new_p.position[1])
        dfa_zs.append(new_p.position[2])

### Mitigated by INFOCOMM


txt_name = "./data/infocomm.txt"

infocomm_xs = []
infocomm_ys = []
infocomm_zs = []
test_traj = []
print(txt_name)
start_time = 0
with open(txt_name) as f:
    lines = f.readlines()
    for line in lines:
        content = [float(x) for x in line.split()]

        new_p = loc_point()
        new_p.timestamp = content[0] / (1.0 * 10e8)
       
        
        if new_p.timestamp > end_timestamp:
            break

        new_p.position = np.array([content[1], content[2], -content[3]])
        new_p.orientation = Quaternion(content[4], content[5], content[6], content[7])

        test_traj.append(new_p)

        infocomm_xs.append(new_p.position[0])
        infocomm_ys.append(new_p.position[1])
        infocomm_zs.append(new_p.position[2])



fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
# Get the current size of the figure
size = fig.get_size_inches()
print("Figure size in inches:", size)

ax.scatter(gtxs[0], gtys[0], gtzs[0], color='grey', edgecolor='black', linewidth=2, s=100, label='Start Point')

ax.plot(gtxs[:gindex], gtys[:gindex], gtzs[:gindex], linewidth=3, color='black', label='Ground Truth')
ax.plot(xs, ys, zs, linewidth=3, color=colors[2], label= 'Basline')
ax.plot(axs, ays, azs, linewidth=3, color=colors[3], label= 'Abnormal Timings')
ax.plot(dfa_xs, dfa_ys, dfa_zs, linewidth=3, color=colors[4], label= 'DFA')
ax.plot(infocomm_xs, infocomm_ys, infocomm_zs, linewidth=3, color=colors[6], label= 'AdapSLAM')
# ax.plot(oxs, oys, ozs, linewidth=1.5, color=colors[0], label= 'Ours in Resource-constrained Platform', path_effects=[pe.Stroke(linewidth=3, foreground='darkslategray'), pe.Normal()])
ax.legend() #fontsize=16)
ax.set_xlabel('X [m]') #, fontsize=16)
ax.set_ylabel('Y [m]') #, fontsize=16)
ax.set_zlabel('Z [m]') #, fontsize=16)
ax.tick_params(axis='both', which='major') #, labelsize=16)
ax.tick_params(axis='both', which='minor') #, labelsize=16)


######### Print the errors




plt.tight_layout()

plt.savefig('dfa_on_orbslam.pdf', format='pdf')
plt.show()
exit(0)
