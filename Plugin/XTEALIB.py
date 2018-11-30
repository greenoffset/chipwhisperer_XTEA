#This is a simple implementation of XTEA
#https://en.wikipedia.org/wiki/XTEA
#This implementation should only be used for research purposes
#It is not optimized for speed or security
#
#This libary is meant to be used for leakage testing using the chipwisperer software
#It is based on https://github.com/OpenXenManager/openxenmanager/blob/master/src/OXM/xtea.py
#Original Author: Paul Chakravarti (paul_dot_chakravarti_at_gmail_dot_com)

class LeakParams:
	keylocation = 0
	preXor = 0
	postXor = 0
	sum = 0
	v = 0
	
def PrintHex(var):
	sep = ""
	s = sep.join(["%02X"%b for b in var])
	return s
	
def PrintLONG(var):
	sep = ""
	s = sep.join(["%08X"%b for b in var])
	return s
def MakeLong(var):
	return long(PrintHex(var),16)
def GetVersion():
	print("This is a test XTEA LIBRARY")
	
def xtea_encrypt(key,block,n=32,endian="!"):
    """
        Decrypt 64 bit data block using XTEA block cypher
        * key = 128 bit (16 char) 
        * block = 64 bit (8 char)
        * n = rounds (default 32)
	"""
    v0,v1 = struct.unpack(endian+"2L",block)
    k = struct.unpack(endian+"4L",key)
    delta,mask = 0x9e3779b9L,0xffffffffL
    sum = (delta * n) & mask
    for round in range(n):
        v1 = (v1 - (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & mask
        sum = (sum - delta) & mask
        v0 = (v0 - (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & mask
    return struct.pack(endian+"2L",v0,v1)

def xtea_decrypt(key,block,n=32):
	"""
		Decrypt 64 bit data block using XTEA block cypher
		* key = 128 bit (16 char) 
		* block = 64 bit (8 char)
		* n = rounds (default 32)
	"""
	v0 = block[0]
	v1 = block[1]
	k = key
	delta,mask = 0x9e3779b9L,0xffffffffL
	sum = (delta * n) & mask
	for round in range(n):
		v1 = (v1 - (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & mask
		sum = (sum - delta) & mask
		v0 = (v0 - (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & mask
	#	print("round: " + str(round) + "- "+str(sum>>11 & 3) + " " +str(sum & 3))  
	return [v0,v1]

	
#we want to leak preformatted values
#we want to leak Xored data
#we want to leak the key location
#we want to leak v data

def xtea_decrypt_leak(key,block,leakRound,n=32):
	"""
		Decrypt 64 bit data block using XTEA block cypher
		* key = 128 bit (16 char)  array of 4 32bits
		* block = 64 bit (8 char)  array of 2 32bits
		* n = rounds (default 32)
	"""
	leak = LeakParams()
	v0 = block[0]
	v1 = block[1]
	k = key
	delta,mask = 0x9e3779b9L,0xffffffffL
	sum = (delta * n) & mask
	round_cnt = 0
	for round in range(n):		
		leak.keylocation = sum>>11 & 3
		leak.preXor = ((v0<<4 ^ v0>>5) + v0)
		leak.postXor =  (leak.preXor ^ (sum + k[sum>>11 & 3]))& mask
		leak.sum = sum
		v1 = (v1 - (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & mask
		leak.v = v1
		if (round_cnt == leakRound):
			return leak
		
		sum = (sum - delta) & mask
		round_cnt = round_cnt + 1

		leak.keylocation = sum & 3
		leak.preXor = ((v1<<4 ^ v1>>5) + v1)
		leak.postXor =  leak.preXor ^ (sum + k[sum & 3])
		leak.sum = sum
		v0 = (v0 - (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & mask
		leak.v = v0
		if (round_cnt == leakRound):
			return leak
		round_cnt = round_cnt + 1
	return [v0,v1]
	"""
	void XTEAdecipher(unsigned int num_rounds, uint32_t * v, uint32_t const key[4]) {
    unsigned int i = 0;
       
    uint32_t v0=v[0], v1=v[1], delta=0x9E3779B9, sum=delta*num_rounds;
    for (i=0; i < num_rounds; i++) {        
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
       
        sum -= delta;
       
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
       
    }
    v[0]=v0; v[1]=v1;
	"""