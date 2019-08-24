#!/usr/bin/env python
"""lab3b.py: Analyzes a file system CSV for inconsistencies."""
__author__ = "Sriram Sonti", "Rohit Falor"

import os
import sys
import csv
from typing import Dict
import collections
from pprint import pprint

from enum import Enum
class BTYPE(Enum):
   DIRECT = ""
   SINGLE = "SINGLE INDIRECT"
   DOUBLE = "DOUBLE INDIRECT"
   TRIPLE = "TRIPLE INDIRECT"

# Define named data collections
SuperBlock = collections.namedtuple(
    'SuperBlock', ('block_count', 'inode_count', 'block_size', 'inode_size', 'blocks_per_group', 'inodes_per_group', 'first_inode'))

Group = collections.namedtuple('Group', ('group_num', 'num_blocks', 'num_inodes', 'num_free_blocks',
                                         'num_free_inodes', 'bg_block_bitmap', 'bg_inode_bitmap', 'bg_inode_table'))

Dirent = collections.namedtuple(
    'Dirent', ('dientnode', 'entry_offset', 'entry_inode', 'rec_len', 'name_len', 'entry_name'))

Inode = collections.namedtuple('Inode', ('inode_num', 'file_type', 'mode', 'owner', 'group', 'link_count',
                                         'change_time', 'mod_time', 'access_time', 'file_size', 'blocks_consumed', 'block_pointers'))

Indirect = collections.namedtuple('Indirect', ('owner_inode_num', 'indirection_level',
                                               'logical_block_offset', 'indirect_block_num', 'referenced_block_num'))

Block = collections.namedtuple(
    'Block', ('valid', 'free', 'reserved', 'references'))

BFREE = []
IFREE = []
DirentData = []
InodeData = {}
IndirectData = []

with open('P3B-test_1.csv') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if(row[0] == "SUPERBLOCK"):
            SuperBlockData = SuperBlock(*map(int, row[1:]))
        elif(row[0] == "GROUP"):
            GroupData = Group(*map(int, row[1:]))
        elif(row[0] == "BFREE"):
            BFREE.append(int(row[1]))
        elif(row[0] == "IFREE"):
            IFREE.append(int(row[1]))
        elif(row[0] == "DIRENT"):
            DirentData.append(Dirent(*row[1:]))
        elif(row[0] == "INODE"):
            temp_inode = Inode(*row[1:12], block_pointers=map(int, row[12:]))
            InodeData[int(temp_inode.inode_num)] = temp_inode
        elif(row[0] == "INDIRECT"):
            IndirectData.append(Indirect(*map(int, row[1:])))

# pprint(SuperBlockData)
# pprint(GroupData)
# pprint(InodeData)

InvalidBlocks = set()
ReservedBlocks = set()

# init reserved block list
ReservedBlocks.add(1)  # SuperBlock
if (SuperBlockData.block_size == 1024):
    ReservedBlocks.add(2)

# free block bitmap, inode bitmap, inode table
ReservedBlocks.add(int(GroupData.bg_block_bitmap))
ReservedBlocks.add(int(GroupData.bg_inode_bitmap))

LegalBlocks = set(range(1, int(SuperBlockData.block_size) + 1)) - ReservedBlocks


inode_table_blocks = (int(SuperBlockData.inode_size) *
                      int(SuperBlockData.inode_count))/(int(SuperBlockData.block_size))
ReservedBlocks.update(range(GroupData.bg_inode_table,
                            GroupData.bg_inode_table + int(inode_table_blocks)))


def is_valid(block_num: int):
    return 0 < block_num <= SuperBlockData.block_count


def is_reserved(block_num: int):
    return (block_num in ReservedBlocks)


# def get_block_offset(level: int):
#     if level <= 11:
#         return 11
#     return pow(num_pointers, level - 1) + get_block_offset(level - 1)


def is_free(block_num: int):
    return (block_num in BFREE)

def get_block_type(index: int):
    if index == 14:
        return BTYPE.TRIPLE
    elif index == 13:
        return BTYPE.DOUBLE
    elif index == 12:
        return BTYPE.SINGLE
    return BTYPE.DIRECT
    
Blocks: Dict[int, Block] = {}

# Iterate Inodes
for inode in InodeData.values():
    for index, block_num in enumerate(inode.block_pointers):
        if(block_num != 0 and index<12):
            block_type = get_block_type(index)
            offset = index

            if block_num in Blocks:
                block = Blocks[block_num]
                block.references.append((inode.inode_num, block_type, offset))

            else:
                Blocks[block_num] = Block(references=[(inode.inode_num, block_type, offset)], free=is_free(block_num),
                                            valid=is_valid(block_num), reserved=is_reserved(block_num))

# Iterate Indirect Blocks
for ind_block in IndirectData:
    block_num = ind_block.referenced_block_num
    
    if(block_num != 0):
        block_type = get_block_type(ind_block.indirection_level + 11)
        offset = ind_block.logical_block_offset
        inode_num = ind_block.owner_inode_num

        if block_num in Blocks:
            block = Blocks[block_num]
            block.references.append((inode_num, block_type, offset))

        else:
            Blocks[block_num] = Block(references=[(inode_num, block_type, offset)], free=is_free(block_num),
                                        valid=is_valid(block_num), reserved=is_reserved(block_num))


# pprint(Blocks)

for block in Blocks.items():
    (block_num, block_data) = block
    if(not block_data.valid):
        refs = block_data.references
        for curr_ref in refs:
            off = 0
            if(curr_ref[1] == ""):
                off = 0
            else:
                off = 0
            print(f"INVALID{curr_ref[1].value} BLOCK {block_num} IN INODE {curr_ref[0]} AT OFFSET {off}")
    
    if(block_data.reserved):
        refs = block_data.references
        for curr_ref in refs:
            off = 0
            if(curr_ref[1] == ""):
                off = 0
            print(f"RESERVED{curr_ref[1].value} BLOCK {block_num} IN INODE {curr_ref[0]} AT OFFSET {off}")

pprint(IndirectData)
