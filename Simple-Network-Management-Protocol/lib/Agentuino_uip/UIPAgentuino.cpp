/*
  Agentuino.cpp - An Arduino library for a lightweight SNMP Agent.
  Copyright (C) 2010 Eric C. Gionet <lavco_eg@hotmail.com>
  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Update by Petr Domorazek <petr@domorazek.cz>

//
// sketch_aug23a
//

#include "UIPAgentuino.h"
#include "UIPUDP.h"
//#include "EthernetUdp.h"


EthernetUDP Udp;
SNMP_API_STAT_CODES AgentuinoClass::begin(){
	// set community names
	_getCommName = "public";
	_setCommName = "private";
	
	// set community name set/get sizes
	_setSize = strlen(_setCommName);
	_getSize = strlen(_getCommName);
	
	// init UDP socket
	Udp.begin(SNMP_DEFAULT_PORT);
	
	return SNMP_API_STAT_SUCCESS;
}

SNMP_API_STAT_CODES AgentuinoClass::begin(char *getCommName, char *setCommName, uint16_t port){
	// set community name set/get sizes
	_setSize = strlen(setCommName);
	_getSize = strlen(getCommName);

	// validate get/set community name sizes
	if ( _setSize > SNMP_MAX_NAME_LEN + 1 || _getSize > SNMP_MAX_NAME_LEN + 1 ) {
		return SNMP_API_STAT_NAME_TOO_BIG;}

	// set community names
	_getCommName = getCommName;
	_setCommName = setCommName;

	// validate session port number
	if ( port == NULL || port == 0 ) port = SNMP_DEFAULT_PORT;

	// init UDP socket
	Udp.begin(port);

	return SNMP_API_STAT_SUCCESS;
}

void AgentuinoClass::listen(void){
	// if bytes are available in receive buffer
	// and pointer to a function (delegate function)
	// isn't null, trigger the function
	Udp.parsePacket();
	if ( Udp.available() && _callback != NULL ) (*_callback)();
}


SNMP_API_STAT_CODES AgentuinoClass::requestPdu(SNMP_PDU *pdu){
	char *community;
	// sequence length
	byte seqLen;
	// version
	byte verLen, verEnd;
	// community string
	byte comLen, comEnd;
	// pdu
	byte pduTyp, pduLen;
	byte ridLen, ridEnd;
	byte errLen, errEnd;
	byte eriLen, eriEnd;
	byte vblTyp, vblLen;
	byte vbiTyp, vbiLen;
	byte obiLen, obiEnd;
	byte valTyp, valLen, valEnd, endVal;
	byte i;

	// set packet packet size (skip UDP header)
	_packetSize = Udp.available();
	
	// reset packet array
	memset(_packet, 0, SNMP_MAX_PACKET_LEN);
	
	// validate packet
	if ( _packetSize != 0 && _packetSize > SNMP_MAX_PACKET_LEN ) {
		//SNMP_FREE(_packet);

		return SNMP_API_STAT_PACKET_TOO_BIG;
	}
	
	// get UDP packet
	//Udp.parsePacket();
	Udp.read(_packet, _packetSize);
	// Udp.readPacket(_packet, _packetSize, _dstIp, &_dstPort);
	
	// packet check 1
	if ( _packet[0] != 0x30 ) {
		//SNMP_FREE(_packet);

		return SNMP_API_STAT_PACKET_INVALID;
	}
	
	// sequence length
	seqLen = _packet[1];
	
	// snmp version
	verLen = _packet[3];
	verEnd = 3 + verLen;
	
	// snmp community string
	comLen = _packet[verEnd + 2];
	comEnd = verEnd + 2 + comLen;
	
	// snmp pdu type
	pduTyp = _packet[comEnd + 1];
	pduLen = _packet[comEnd + 2];
	
	// requestId
	ridLen = _packet[comEnd + 4];
	ridEnd = comEnd + 4 + ridLen;

	// error
	errLen = _packet[ridEnd + 2];
	errEnd = ridEnd + 2 + errLen;
	
	// error index
	eriLen = _packet[errEnd + 2];
	eriEnd = errEnd + 2 + eriLen;
	
	// varbind list
	vblTyp = _packet[eriEnd + 1];
	vblLen = _packet[eriEnd + 2];
	
	// varbind
	vbiTyp = _packet[eriEnd + 3];
	vbiLen = _packet[eriEnd + 4];
	
	// extract version
	pdu->version = 0;
	for ( i = 0; i < verLen; i++ ) {
		pdu->version = (pdu->version << 8) | _packet[5 + i];
	}
	
	// validate version
	
	// pdu-type
	pdu->type = (SNMP_PDU_TYPES)pduTyp;
	_dstType = pdu->type;
	
	// validate community size
	if ( comLen > SNMP_MAX_NAME_LEN ) {
		// set pdu error
		pdu->error = SNMP_ERR_TOO_BIG;
		
		return SNMP_API_STAT_NAME_TOO_BIG;
	}
	
	// validate community name
	if ( pdu->type == SNMP_PDU_SET && comLen == _setSize ) {
		for ( i = 0; i < _setSize; i++ ) {
			if( _packet[verEnd + 3 + i] != (byte)_setCommName[i] ) {
				// set pdu error
				pdu->error = SNMP_ERR_NO_SUCH_NAME;
				
				return SNMP_API_STAT_NO_SUCH_NAME;
			}
		}
	} 
	else if ( pdu->type == SNMP_PDU_GET && comLen == _getSize ) {
		
		for ( i = 0; i < _getSize; i++ ) {
			if( _packet[verEnd + 3 + i] != (byte)_getCommName[i] ) {
				// set pdu error
				pdu->error = SNMP_ERR_NO_SUCH_NAME;
				
				return SNMP_API_STAT_NO_SUCH_NAME;
			}
		}
	} 
	else if ( pdu->type == SNMP_PDU_GET_NEXT && comLen == _getSize ) {
		for ( i = 0; i < _getSize; i++ ) {
			if( _packet[verEnd + 3 + i] != (byte)_getCommName[i] ) {
				// set pdu error
				pdu->error = SNMP_ERR_NO_SUCH_NAME;
				
				return SNMP_API_STAT_NO_SUCH_NAME;
			}
		}
	} 
	else {
		// set pdu error
		pdu->error = SNMP_ERR_NO_SUCH_NAME;
		
		return SNMP_API_STAT_NO_SUCH_NAME;
	}
	
	// extract reqiest-id 0x00 0x00 0x00 0x01 (4-byte int aka int32)
	pdu->requestId = 0;
	for ( i = 0; i < ridLen; i++ ) {
		pdu->requestId = (pdu->requestId << 8) | _packet[comEnd + 5 + i];
	}
	
	// extract error 
	pdu->error = SNMP_ERR_NO_ERROR;
	int32_t err = 0;
	for ( i = 0; i < errLen; i++ ) {
		err = (err << 8) | _packet[ridEnd + 3 + i];
	}
	pdu->error = (SNMP_ERR_CODES)err;
	
	// extract error-index
	pdu->errorIndex = 0;
	for ( i = 0; i < eriLen; i++ ) {
		pdu->errorIndex = (pdu->errorIndex << 8) | _packet[errEnd + 3 + i];
	}
	
	while(1){
		// define OID and Value from packet

		// validate object-identifier size
		if ( obiLen > SNMP_MAX_OID_LEN ) {
			// set pdu error
			pdu->error = SNMP_ERR_TOO_BIG;

			return SNMP_API_STAT_OID_TOO_BIG;}
		
		// oid
		if(_numberOID == 0){
			obiLen = _packet[eriEnd + 6];
			obiEnd = eriEnd + 6 + obiLen;}
		else{
			obiLen = _packet[valEnd + 4];
			obiEnd = valEnd + 4 + obiLen;}

		//value
		valTyp = _packet[obiEnd + 1];
		valLen = _packet[obiEnd + 2];
		valEnd = obiEnd + 2 + valLen;
		
		// extract and contruct object-identifier
		memset(pdu->OID[_numberOID].data, 0, SNMP_MAX_OID_LEN);
		pdu->OID[_numberOID].size = obiLen;
		for ( i = 0; i < obiLen; i++ ) {
			if(_numberOID == 0){
				pdu->OID[_numberOID].data[i] = _packet[eriEnd + 7 + i];}
			else{
				pdu->OID[_numberOID].data[i] = _packet[endVal + 5 + i];}
		}
			
		// value-type
		pdu->VALUE[_numberOID].syntax = (SNMP_SYNTAXES)valTyp;
		
		// validate value size
		if ( obiLen > SNMP_MAX_VALUE_LEN ) {
			// set pdu error
			pdu->error = SNMP_ERR_TOO_BIG;

			return SNMP_API_STAT_VALUE_TOO_BIG;
		}

		// value-size
		pdu->VALUE[_numberOID].size = valLen;

		// extract value
		memset(pdu->VALUE[_numberOID].data, 0, SNMP_MAX_VALUE_LEN);
		for ( i = 0; i < valLen; i++ ) {
			pdu->VALUE[_numberOID].data[i] = _packet[obiEnd + 3 + i];}
		
		// variable compare with total OID
		if(_packetSize == valEnd+1){	//Stop OID Increment
			pdu->SNMP_TOTAL_OID = _numberOID;
			_numberOID = 0;
			endVal = 0;
			break;}
		else{							//OID Increment
			_numberOID += 1;
			endVal = valEnd;}
	}
	
	return SNMP_API_STAT_SUCCESS;
}

SNMP_API_STAT_CODES AgentuinoClass::responsePdu(SNMP_PDU *pdu){
	int32_u u;	
	byte i;
	total=0;
	
	// Length of entire SNMP packet
	_packetPos = 0;  // 23
	
	/*for(int count=0; count<=9; count++){
		totalOID = totalOID + pdu->OID[count].size;
	}
	for(int count=0; count<=9; count++){
		totalVAL = totalVAL + pdu->VALUE[count].size;
	}
	total = totalVAL + totalOID;*/
	
	for(int count=0; count<=pdu->SNMP_TOTAL_OID; count++){
		total = total + pdu->VALUE[count].size + pdu->OID[count].size;}

	//_packetSize = 16 + 7 + sizeof(pdu->requestId) + (sizeof(pdu->error) + 3) + sizeof(pdu->errorIndex) + (total + 6*(pdu->SNMP_TOTAL_OID));
	_packetSize = 16 + 1 + sizeof(pdu->requestId) + (sizeof(pdu->error) + 3) + sizeof(pdu->errorIndex) + (total + 6*(pdu->SNMP_TOTAL_OID+1));

	memset(_packet, 0, SNMP_MAX_PACKET_LEN);
	
	if ( _dstType == SNMP_PDU_SET ) {
		_packetSize += _setSize;
	} 
	else {
		_packetSize += _getSize;
	}
	
	// SNMP MESSAGE TYPE
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_SEQUENCE;	// type
	_packet[_packetPos++] = (byte)_packetSize - 2;		// length
	
	// SNMP version
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_INT;	// type
	_packet[_packetPos++] = 0x01;			// length
	_packet[_packetPos++] = 0x00;			// value
	
	// SNMP community string
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_OCTETS;	// type
	if ( _dstType == SNMP_PDU_SET ) {
		_packet[_packetPos++] = (byte)_setSize;	// length
		for ( i = 0; i < _setSize; i++ ) {
			_packet[_packetPos++] = (byte)_setCommName[i];
		}
	} 
	else {
		_packet[_packetPos++] = (byte)_getSize;	// length
		for ( i = 0; i < _getSize; i++ ) {
			_packet[_packetPos++] = (byte)_getCommName[i];
		}
	}
	
	// SNMP PDU TYPE
	_packet[_packetPos++] = (byte)pdu->type;
	//_packet[_packetPos++] = (byte)( sizeof(pdu->requestId) + sizeof((int32_t)pdu->error) + sizeof(pdu->errorIndex) + total + 14);
	if ( _dstType == SNMP_PDU_GET ) {
		_packet[_packetPos++] = (byte)(_packetSize - (6 + 1 + 2 + _getSize));
	}
	else{
		_packet[_packetPos++] = (byte)(_packetSize - (6 + 1 + _setSize));
	}
	
	// Request ID (size always 4 e.g. 4-byte int)
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_INT;	// type
	_packet[_packetPos++] = (byte)sizeof(pdu->requestId);
	u.int32 = pdu->requestId;
	_packet[_packetPos++] = u.data[3];
	_packet[_packetPos++] = u.data[2];
	_packet[_packetPos++] = u.data[1];
	_packet[_packetPos++] = u.data[0];
	
	// Error Status (size always 4 e.g. 4-byte int)
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_INT;	// type
	_packet[_packetPos++] = (byte)sizeof((int32_t)pdu->error);
	u.int32 = pdu->error;
	_packet[_packetPos++] = u.data[3];
	_packet[_packetPos++] = u.data[2];
	_packet[_packetPos++] = u.data[1];
	_packet[_packetPos++] = u.data[0];
	
	// Error Index (size always 4 e.g. 4-byte int)
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_INT;	// type
	_packet[_packetPos++] = (byte)sizeof(pdu->errorIndex);
	u.int32 = pdu->errorIndex;
	_packet[_packetPos++] = u.data[3];
	_packet[_packetPos++] = u.data[2];
	_packet[_packetPos++] = u.data[1];
	_packet[_packetPos++] = u.data[0];

	// Varbind List
	_packet[_packetPos++] = (byte)SNMP_SYNTAX_SEQUENCE;	// type
	_packet[_packetPos++] = (byte)(total + 6*((pdu->SNMP_TOTAL_OID)+1)); //4
	
	// Many ObjectIdentifier and Value
	for(int count=0; count<= pdu->SNMP_TOTAL_OID; count++){
		// Varbind List
		//_packet[_packetPos++] = (byte)SNMP_SYNTAX_SEQUENCE;	// type
		//_packet[_packetPos++] = (byte)(total + 6*((pdu->SNMP_TOTAL_OID)+1)); //4
		
		// Varbind
		_packet[_packetPos++] = (byte)SNMP_SYNTAX_SEQUENCE;	// type
		_packet[_packetPos++] = (byte)(pdu->VALUE[count].size + pdu->OID[count].size + 4 ); //2

		// ObjectIdentifier
		_packet[_packetPos++] = (byte)SNMP_SYNTAX_OID;	// type
		_packet[_packetPos++] = (byte)(pdu->OID[count].size);

		for ( i = 0; i < pdu->OID[count].size; i++ ) {
			_packet[_packetPos++] = pdu->OID[count].data[i];			
		}

		// Value
		_packet[_packetPos++] = (byte)(pdu->VALUE[count].syntax);	// type
		_packet[_packetPos++] = (byte)(pdu->VALUE[count].size);
		
		for ( i = 0; i < pdu->VALUE[count].size; i++ ) {
			_packet[_packetPos++] = pdu->VALUE[count].data[i];
		}
	}	
	
	Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
	Udp.write(_packet, _packetSize);
	Udp.endPacket();
	//Udp.write(_packet, _packetSize, _dstIp, _dstPort);
	//
	Udp.stop();
	Udp.begin(SNMP_DEFAULT_PORT);

	return SNMP_API_STAT_SUCCESS;
}

void AgentuinoClass::onPduReceive(onPduReceiveCallback pduReceived){
	_callback = pduReceived;
}

void AgentuinoClass::freePdu(SNMP_PDU *pdu){
	for(int count=0; count<=9; count++){
		memset(pdu->OID[count].data, 0, SNMP_MAX_OID_LEN);
		memset(pdu->VALUE[count].data, 0, SNMP_MAX_VALUE_LEN);
		//free((char *) pdu);
	}
	
	return;
}

void AgentuinoClass::freeTrapPdu(SNMP_TRAP_PDU *pdu){
	memset(pdu->TrapOID.data, 0, SNMP_MAX_OID_LEN);
	memset(pdu->EnterpriseTrapOID.data, 0, SNMP_MAX_VALUE_LEN);
	//free((char *) pdu);
	
	return;
}

// Create one global object
AgentuinoClass Agentuino;

void AgentuinoClass::Trap(char Message[], byte RemIP[4], byte AgntIP[4], uint32_t Tiempo, SNMP_TRAP_PDU *pduu){
	_packetPosTrap = 0;
	memset(_packetTrap, 0, SNMP_MAX_PACKET_LEN);
	
	_packetSizeTrap = 49 + pduu->EnterpriseTrapOID.size + pduu->TrapOID.size + strlen(Message);
	
	// SNMP MESSAGE TYPE
	_packetTrap[_packetPosTrap++] = (byte)SNMP_SYNTAX_SEQUENCE; // type
	_packetTrap[_packetPosTrap++] = (byte)(_packetSizeTrap-2);  // length

	// SNMP version
	_packetTrap[_packetPosTrap++] = (byte)SNMP_SYNTAX_INT;  // type
	_packetTrap[_packetPosTrap++] = 0x01;     // length
	_packetTrap[_packetPosTrap++] = 0x00;     // value
	
	// SNMP community string
	_packetTrap[_packetPosTrap++] = (byte)SNMP_SYNTAX_OCTETS; // type
	_packetTrap[_packetPosTrap++] = (byte)_getSize; // length
	for (int i = 0; i < _getSize; i++ ){
		_packetTrap[_packetPosTrap++] = (byte)_getCommName[i];}

	// SNMP PDU TYPE
	_packetTrap[_packetPosTrap++] = (byte)164;
	_packetTrap[_packetPosTrap++] = (byte)130;
	_packetTrap[_packetPosTrap++] = (byte)0;
	_packetTrap[_packetPosTrap++] = (byte)(32 + pduu->EnterpriseTrapOID.size + pduu->TrapOID.size + strlen(Message));
	
	// Enterprise OID
	_packetTrap[_packetPosTrap++] = (byte)6;
	_packetTrap[_packetPosTrap++] = (byte)(pduu->EnterpriseTrapOID.size);
	for (int i = 0; i < pduu->EnterpriseTrapOID.size; i++){
		_packetTrap[_packetPosTrap++] = pduu->EnterpriseTrapOID.data[i];}
		
	// IP Agent Address
	_packetTrap[_packetPosTrap++] = (byte)64;
	_packetTrap[_packetPosTrap++] = (byte)4;
	for (int i = 0; i <= 3; i++){
		_packetTrap[_packetPosTrap++] = (byte)AgntIP[i];}
	
	//Generic Trap
	_packetTrap[_packetPosTrap++] = (byte)(2);
	_packetTrap[_packetPosTrap++] = (byte)(1);
	_packetTrap[_packetPosTrap++] = (byte)(6);
  
	//Specific Trap
	_packetTrap[_packetPosTrap++] = (byte)(2);
	_packetTrap[_packetPosTrap++] = (byte)(1);
	_packetTrap[_packetPosTrap++] = (byte)(1);
	
	//Timestamp
	_packetTrap[_packetPosTrap++] = (byte)(67);
	_packetTrap[_packetPosTrap++] = (byte)(4);
	
	// The next part is to change the locUpTime into bytes
	int i=0, 
		k=1,
		temp;
	byte 	suma = 0,
			hexadecimalNumber[4] ={0, 0, 0, 0};
	uint32_t quotient = Tiempo;

	while (quotient!=0){
		temp = quotient % 16;
		if (k == 1){
			suma = temp;
			k = 2;}
		else{
			suma = suma + temp*16;
			hexadecimalNumber[3-i] = suma;
			i = i + 1;
			k = 1;}
		quotient = quotient / 16;}
	if (k == 2){
		hexadecimalNumber[3-i] = suma;}
		
	for (int i = 0; i < 4; i++){
		_packetTrap[_packetPosTrap++] = hexadecimalNumber[i];}
	
	//Varbind
	_packetTrap[_packetPosTrap++] = (byte)(48);
	_packetTrap[_packetPosTrap++] = (byte)(130);
	_packetTrap[_packetPosTrap++] = (byte)(0);
	_packetTrap[_packetPosTrap++] = (byte)(8 + pduu->TrapOID.size + strlen(Message));
    
	//Varbind List
	_packetTrap[_packetPosTrap++] = (byte)(48);
	_packetTrap[_packetPosTrap++] = (byte)(130);
	_packetTrap[_packetPosTrap++] = (byte)(0);
	_packetTrap[_packetPosTrap++] = (byte)(4 + pduu->TrapOID.size +  strlen(Message));
	
	//OID 1
	_packetTrap[_packetPosTrap++] = (byte)6;
	_packetTrap[_packetPosTrap++] = (byte)(pduu->TrapOID.size);
	for (int i = 0; i < pduu->TrapOID.size; i++){
		_packetTrap[_packetPosTrap++] = pduu->TrapOID.data[i];}
	
	//Value
	_packetTrap[_packetPosTrap++] = (byte)4;
	_packetTrap[_packetPosTrap++] = (byte)(strlen(Message));
	for (int i = 0; i < strlen(Message); i++){
		_packetTrap[_packetPosTrap++] = (byte)Message[i];}
	
	Udp.beginPacket(RemIP, 162);   // Here is defined the UDP port 162 to send the trap
	Udp.write(_packetTrap, _packetSizeTrap);
	Udp.endPacket();
	
	Udp.stop();
	Udp.begin(SNMP_DEFAULT_PORT);
}