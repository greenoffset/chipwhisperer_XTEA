#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2017, NewAE Technology Inc
# All rights reserved.
#
# Authors: Colin O'Flynn , Greenoffset
#
# This code is based on newae XOR attack model
# Modified to implement XTEA decryption attack in 2018
# This project was also submitted for the newAE contest
#
# Find this and more at newae.com - this file is part of the chipwhisperer
# project, http://www.assembla.com/spaces/chipwhisperer
#
#    This file is part of chipwhisperer.
#
#    chipwhisperer is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    chipwhisperer is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with chipwhisperer.  If not, see <http://www.gnu.org/licenses/>.

from collections import OrderedDict
import inspect

from base import ModelsBase
from chipwhisperer.common.utils.pluginmanager import Plugin
from chipwhisperer.common.utils.parameter import setupSetParam
import XTEALIB as xt
import re
import numpy as np

#Yes this may be stupid, but i'm an embedded guy and see trough this easyer
#Functions to pack from [8bit *4] to 32 bit
def Unpack32(in32):
    result = []
    result.append((in32 >> 8*3) & 0xFF)
    result.append((in32 >> 8*2) & 0xFF)
    result.append((in32 >> 8*1) & 0xFF)
    result.append((in32 >> 8*0) & 0xFF)
    return result
#Function to unpack from 32bit to [8bit *4]
def Pack32(in_array4):
    return (np.int64(in_array4[0]) << 8*3)|(np.int64(in_array4[1]) << 8*2)|(np.int64(in_array4[2]) << 8*1)|(np.int64(in_array4[3]) << 8*0)

class XORLeakageHelper(object):

    name = 'XOR Leakage Model (unnamed)'

    def leakage(self, pt, ct, key, bnum):
        """
        Override this function with specific leakage function (S-Box output, HD, etc).

        Args:
            pt: 16-byte plain-text input
            ct: 16-byte cipher-text output.
            key: 16-byte AES key - byte 'bnum' may be a GUESS if key is known. Rest of bytes may/may not be valid too.
            bnum: Byte number we are trying to attack.

        Returns:
            Value that will be presented on the 8-bit bus. Leakage model (such as HW) will map this to leakage itself.
        """
        raise NotImplementedError("ASKLeakageHelper does not implement leakage")

class PtKey_XTEA(XORLeakageHelper):
    name = 'HW: XTEA(pt,key) pt padded'
    def leakage(self, pt, ct, key, bnum):
        #print key
        lkey = []
        lkey.append(Pack32(key[0:4]))
        lkey.append(Pack32(key[4:8]))
        lkey.append(Pack32(key[8:12]))
        lkey.append(Pack32(key[12:16]))
       # print lkey
        lpt = []
        lpt.append(Pack32(pt[0:4]))
        lpt.append(Pack32(pt[4:8]))
        #lkey = [0xf8e9ebde, 0x53ce00f7,0x2fe12fe3, 0x04dcee29 ]
            
        lik = xt.xtea_decrypt_leak(lkey,lpt,bnum)

        return lik.v

#List of all classes you can use
enc_list = [PtKey_XTEA]
dec_list = []

class XTEA_32(ModelsBase, Plugin):
    _name = 'XTEA_32'

    hwModels = OrderedDict((mod.name, mod) for mod in (enc_list+dec_list) )
    #We use static variables only becuse is simpler. I already said, im not an OOP guy
    leak_round = [None]*4 #This variable will sotre the leak location for the 4 32bit subkeys
    AttackedRound = 0
    AttackedKey = 0
    GUIKnownKeys = [None]*4
    Padding = 0


    UserInterfaceList = []
    cn = 0
    def updateParameters(self,_=None):      
        XTEA_32.AttackedRound = int(re.search(r'\d+',self.findParam('subkey').getValue()).group())
        XTEA_32.AttackedKey = XTEA_32.leak_round.index(self.AttackedRound)
        XTEA_32.GUIKnownKeys[0] = long(self.findParam('known_key_0').getValue(),16)
        XTEA_32.GUIKnownKeys[1] = long(self.findParam('known_key_1').getValue(),16)
        XTEA_32.GUIKnownKeys[2] = long(self.findParam('known_key_2').getValue(),16)
        XTEA_32.GUIKnownKeys[3] = long(self.findParam('known_key_3').getValue(),16)

        if "Rigth" in self.findParam("padding").getValue():
            XTEA_32.Padding = 1 #Padding to the rigth           
        else:
            XTEA_32.Padding = 0 #Padding to the left
    def __init__(self, model=PtKey_XTEA, bitmask=0xFF):
        ModelsBase.__init__(self, 4, 256, model=model)

        #Override number of subkeys - todo make this work
        #self.numSubKeys = 2
        #self.getParams().addChildren([{'name': 'Number of SubKeys', 'type':'list', 'values':[1,2,4,8,16,32], 'get':self.getNumSubKeys, 'set':self.setNumSubKeys}])
        
        #We want to calculate for each 32bit subkey in which round is first used. This could be a constant, but if someone messes with the constants in the 
        #algorithm then a hardcode here would be bad
        for round in range(32):
            used_key = xt.xtea_decrypt_leak([0, 0, 0, 0],[0, 0],round).keylocation
            if XTEA_32.leak_round[used_key] == None:
                XTEA_32.leak_round[used_key] = round
        #print(XTEA_32.leak_round)
        #because the subkeys are used one after the other we have to guess the first used key because the others depend on it
        XTEA_32.UserInterfaceList = []
        for i in range(4):
            XTEA_32.UserInterfaceList.append("Round " + str(self.leak_round[i]) + " Key " + str(i))
        XTEA_32.UserInterfaceList.sort()
        #print(XTEA_32.UserInterfaceList)
        #for round in self.leak_round.sort():
        #    print (round)
        self.params.addChildren([
        {'name':'Subkey Attacked', 'key':'subkey','type':'list', 'values':self.UserInterfaceList,'value':self.UserInterfaceList[0], 'action':self.updateParameters},
        {'name':'Known KEY 0', 'key':'known_key_0', 'type':'str', 'value':"FFFFFFFF", 'action':self.updateParameters},
        {'name':'Known KEY 1', 'key':'known_key_1', 'type':'str', 'value':"FFFFFFFF", 'action':self.updateParameters},
        {'name':'Known KEY 2', 'key':'known_key_2', 'type':'str', 'value':"FFFFFFFF", 'action':self.updateParameters},
        {'name':'Known KEY 3', 'key':'known_key_3', 'type':'str', 'value':"FFFFFFFF", 'action':self.updateParameters},
        {'name':'Input padding', 'key':'padding', 'type':'list','values':["Left (0,in)","Rigth (in,0)"], 'value':"Rigth (in,0)", 'action':self.updateParameters}       
        ])
        #Check if the known keys are not initialized initialize them
        if XTEA_32.GUIKnownKeys[0] == None:
            self.updateParameters()
        self._mask = bitmask


    def _updateHwModel(self):
        """" Re-implement this to update leakage model """
        self.modelobj = None

        #Check if they passed an object...
        if isinstance(self.model, XORLeakageHelper):
            self.modelobj = self.model

        #Check if they passed a class...
        elif inspect.isclass(self.model) and issubclass(self.model, XORLeakageHelper):
            self.modelobj = self.model()

        #Otherwise it's probably one of these older keys (kept for backwards-compatability)
        else:
            raise AttributeError("Unknown model")

        if self.modelobj is None:
            raise ValueError("Invalid model: %s" % str(self.model))

    def processKnownKey(self, inpkey):        
        #Because we attacking only 4bytes at a time, this ensures correct known key highligthing
        return inpkey[XTEA_32.AttackedKey*4:(XTEA_32.AttackedKey*4+4)]

    def leakage(self, pt, ct, guess, bnum, state):        
        try:
            #Make a copy so we don't screw with anything...
            key = list(state['knownkey'])
        except:
            #We don't log due to time-sensitive nature... but if state doesn't have "knownkey" will result in
            #unknown knownkey which causes some attacks to fail. Possibly should make this some sort of
            #flag to indicate we want to ignore the problem?
            key = [None]*16
        
        #Here we have to take apart [Key] from 32bits to 8 bits to comply with logic
        key = []
        
        for known in XTEA_32.GUIKnownKeys:
            key = key + Unpack32(known)
        
        #Guess can be 'none' if we want to use original key as-is
        if guess is not None:                        
            key[bnum + XTEA_32.AttackedKey*4] = guess
     
        if XTEA_32.Padding == 1:
            pt = np.concatenate((pt,[0,0,0,0]), axis=0)            
        else:
            pt = np.concatenate(([0,0,0,0],pt), axis=0)
      
        #Get intermediate value
        #intermediate_value = self.modelobj.leakage(pt, ct, key, bnum)        
        if isinstance(self.modelobj, PtKey_XTEA): #We check if we are using ptKey_XTEA because we are using the bnum as a hack to transfer lekeageround
            intermediate_value = self.modelobj.leakage(pt, ct, key, XTEA_32.AttackedRound)
        else:
            intermediate_value = self.modelobj.leakage(pt, ct, key, bnum)
        #print intermediate_value
        #For bit-wise attacks, mask off specific bit value
        #intermediate_value = self._mask & intermediate_value
        
        #Return HW of guess
        #Using this HW calculation because we need to calculate HW of 32bits
        return bin(intermediate_value).count("1")# self.HW[intermediate_value]
