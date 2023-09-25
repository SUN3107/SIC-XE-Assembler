#include "pass1.cpp"

using namespace std;

ifstream intermediateFile;                           //Declare global variables
ofstream errorFile, objectFile, ListingFile;

ofstream printtab;
string writestring;

bool isComment;
string label, opcode, operand, comment;
string operand1, operand2;

int lineNumber, blockNumber, address, startAddress;
string objectCode, writeData, currentRecord, modificationRecord = "M^", endRecord, write_R_Data, write_D_Data, currentSectName = "DEFAULT";
int sectionCounter = 0;
int program_section_length = 0;

int program_counter, base_register_value, currentTextRecordLength;
bool nobase;

string readTillTab(string data, int &index)
{
  string tempBuffer = "";

  while (index < data.length() && data[index] != '\t')
  {
    tempBuffer += data[index];
    index++;
  }
  index++;
  if (tempBuffer == " ")
  {
    tempBuffer = "-1";
  }
  return tempBuffer;
}
bool readIntermediateFile(ifstream &readFile, bool &isComment, int &lineNumber, int &address, int &blockNumber, string &label, string &opcode, string &operand, string &comment)
{
  string fileLine = "";
  string tempBuffer = "";
  bool tempStatus;
  int index = 0;
  if (!getline(readFile, fileLine))
  {
    return false;
  }
  lineNumber = stoi(readTillTab(fileLine, index));

  isComment = (fileLine[index] == '.') ? true : false;
  if (isComment)
  {
    read_First_Non_White_Space(fileLine, index, tempStatus, comment, true);
    return true;
  }
  address = string_Hex_To_Int(readTillTab(fileLine, index));
  tempBuffer = readTillTab(fileLine, index);
  if (tempBuffer == " ")
  {
    blockNumber = -1;
  }
  else
  {
    blockNumber = stoi(tempBuffer);
  }
  label = readTillTab(fileLine, index);
  if (label == "-1")
  {
    label = " ";
  }
  opcode = readTillTab(fileLine, index);
  if (opcode == "BYTE")
  {
    read_Byte_Operand(fileLine, index, tempStatus, operand);
  }
  else
  {
    operand = readTillTab(fileLine, index);
    if (operand == "-1")
    {
      operand = " ";
    }
    if (opcode == "CSECT")
    {
      return true;
    }
  }
  read_First_Non_White_Space(fileLine, index, tempStatus, comment, true);
  return true;
}

string createObjectCodeFormat34()
{
  string objcode;
  int halfBytes;
  halfBytes = (get_Flag_Format(opcode) == '+') ? 5 : 3;

  if (get_Flag_Format(operand) == '#')
  { // Immediate
    if (operand.substr(operand.length() - 2, 2) == ",X")
    { // Error handling for Immediate with index based
      writeData = "Line: " + to_string(lineNumber) + " Indirect addressing does not support Index based addressing. ";
      write_To_File(errorFile, writeData);
      objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
      objcode += (halfBytes == 5) ? "100000" : "0000";
      return objcode;
    }

    string tempOperand = operand.substr(1, operand.length() - 1);
    if (if_all_num(tempOperand) || ((SYMTAB[tempOperand].exists == 'y') && (SYMTAB[tempOperand].relative == 0)))
    { // Immediate integer value
      int immediateValue;

      if (if_all_num(tempOperand))
      {
        immediateValue = stoi(tempOperand);
      }
      else
      {
        immediateValue = string_Hex_To_Int(SYMTAB[tempOperand].address);
      }
      //Process Immediate value
      if (immediateValue >= (1 << 4 * halfBytes))
      { 
        writeData = "Line: " + to_string(lineNumber) + " Format limit exceeded by immediate value";
        write_To_File(errorFile, writeData);
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
        objcode += (halfBytes == 5) ? "100000" : "0000";
      }
      else
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
        objcode += (halfBytes == 5) ? '1' : '0';
        objcode += int_ToString_Hex(immediateValue, halfBytes);
      }
      return objcode;
    }
    else if (SYMTAB[tempOperand].exists == 'n')
    {

      if (halfBytes == 3)
      {
        writeData = "Line " + to_string(lineNumber);
        if (halfBytes == 3)
        {
          writeData += " : Invalid format for external reference " + tempOperand;
        }
        else
        {
          writeData += " : Found " + tempOperand + ": Doesn't Exist";
        }
        write_To_File(errorFile, writeData);
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
        objcode += (halfBytes == 5) ? "100000" : "0000";
        return objcode;
      }

    }
    else
    {
      int operandAddress = string_Hex_To_Int(SYMTAB[tempOperand].address) + string_Hex_To_Int(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);

      /*Process Immediate symbol value*/
      if (halfBytes == 5)
      { /*If format 4*/
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
        objcode += '1';
        objcode += int_ToString_Hex(operandAddress, halfBytes);

        /*this add modification record here*/
        modificationRecord += "M^" + int_ToString_Hex(address + 1, 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objcode;
      }

      /*Handle format 3*/
      program_counter = address + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
      program_counter += (halfBytes == 5) ? 4 : 3;
      int relativeAddress = operandAddress - program_counter;

      /*Try PC based*/
      if (relativeAddress >= (-2048) && relativeAddress <= 2047)
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
        objcode += '2';
        objcode += int_ToString_Hex(relativeAddress, halfBytes);
        return objcode;
      }

      
      if (!nobase)
      {
        relativeAddress = operandAddress - base_register_value;
        if (relativeAddress >= 0 && relativeAddress <= 4095)
        {
          objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
          objcode += '4';
          objcode += int_ToString_Hex(relativeAddress, halfBytes);
          return objcode;
        }
      }

      /*Use direct addressing with no PC or base*/
      if (operandAddress <= 4095)
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 1, 2);
        objcode += '0';
        objcode += int_ToString_Hex(operandAddress, halfBytes);

        /*add modification record here*/
        modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objcode;
      }
    }
  }
  else if (get_Flag_Format(operand) == '@')
  {
    string tempOperand = operand.substr(1, operand.length() - 1);
    if (tempOperand.substr(tempOperand.length() - 2, 2) == ",X" || SYMTAB[tempOperand].exists == 'n')
    { // Error handling for Indirect with index based

      
      if (halfBytes == 3)
      {
        writeData = "Line " + to_string(lineNumber);
        
        writeData += " : Symbol doesn't exist. Indirect addressing does not support Index based addressing ";
       
        write_To_File(errorFile, writeData);
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 2, 2);
        objcode += (halfBytes == 5) ? "100000" : "0000";
        return objcode;
      }
      
    }
    int operandAddress = string_Hex_To_Int(SYMTAB[tempOperand].address) + string_Hex_To_Int(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);
    program_counter = address + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    program_counter += (halfBytes == 5) ? 4 : 3;

    if (halfBytes == 3)
    {
      int relativeAddress = operandAddress - program_counter;
      if (relativeAddress >= (-2048) && relativeAddress <= 2047)
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 2, 2);
        objcode += '2';
        objcode += int_ToString_Hex(relativeAddress, halfBytes);
        return objcode;
      }

      if (!nobase)
      {
        relativeAddress = operandAddress - base_register_value;
        if (relativeAddress >= 0 && relativeAddress <= 4095)
        {
          objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 2, 2);
          objcode += '4';
          objcode += int_ToString_Hex(relativeAddress, halfBytes);
          return objcode;
        }
      }

      if (operandAddress <= 4095)
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 2, 2);
        objcode += '0';
        objcode += int_ToString_Hex(operandAddress, halfBytes);

        /*add modification record here*/
        modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objcode;
      }
    }
    else
    { // No base or pc based addressing in format 4
      objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 2, 2);
      objcode += '1';
      objcode += int_ToString_Hex(operandAddress, halfBytes);

      /*add modification record here*/
      modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      modificationRecord += (halfBytes == 5) ? "05" : "03";
      modificationRecord += '\n';

      return objcode;
    }

    writeData = "Line: " + to_string(lineNumber);
    writeData += "Can not fit into program counter based or base register based addressing.";
    write_To_File(errorFile, writeData);
    objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 2, 2);
    objcode += (halfBytes == 5) ? "100000" : "0000";

    return objcode;
  }
  else if (get_Flag_Format(operand) == '=')
  { 
    string tempOperand = operand.substr(1, operand.length() - 1);

    if (tempOperand == "*")
    {
      tempOperand = "X'" + int_ToString_Hex(address, 6) + "'";
      modificationRecord += "M^" + int_ToString_Hex(string_Hex_To_Int(LITTAB[tempOperand].address) + string_Hex_To_Int(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].startAddress), 6) + '^';
      modificationRecord += int_ToString_Hex(6, 2);
      modificationRecord += '\n';
    }

    if (LITTAB[tempOperand].exists == 'n')
    {
      writeData = "Line " + to_string(lineNumber) + " : Found " + tempOperand + "Doesn't exist";
      write_To_File(errorFile, writeData);

      objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
      objcode += (halfBytes == 5) ? "000" : "0";
      objcode += "000";
      return objcode;
    }

    int operandAddress = string_Hex_To_Int(LITTAB[tempOperand].address) + string_Hex_To_Int(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].startAddress);
    program_counter = address + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
    program_counter += (halfBytes == 5) ? 4 : 3;

    if (halfBytes == 3)
    {
      int relativeAddress = operandAddress - program_counter;
      if (relativeAddress >= (-2048) && relativeAddress <= 2047)
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
        objcode += '2';
        objcode += int_ToString_Hex(relativeAddress, halfBytes);
        return objcode;
      }

      if (!nobase)
      {
        relativeAddress = operandAddress - base_register_value;
        if (relativeAddress >= 0 && relativeAddress <= 4095)
        {
          objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
          objcode += '4';
          objcode += int_ToString_Hex(relativeAddress, halfBytes);
          return objcode;
        }
      }

      if (operandAddress <= 4095)
      {
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
        objcode += '0';
        objcode += int_ToString_Hex(operandAddress, halfBytes);

        /*add modification record here*/
        modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objcode;
      }
    }
    else
    { // No base or pc based addressing in format 4
      objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
      objcode += '1';
      objcode += int_ToString_Hex(operandAddress, halfBytes);

      /*add modification record here*/
      modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      modificationRecord += (halfBytes == 5) ? "05" : "03";
      modificationRecord += '\n';

      return objcode;
    }

    writeData = "Line: " + to_string(lineNumber);
    writeData += "Can not fit into program counter based or base register based addressing.";
    write_To_File(errorFile, writeData);
    objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
    objcode += (halfBytes == 5) ? "100" : "0";
    objcode += "000";

    return objcode;
  }
  else
  { //direct addressing
    int xbpe = 0;
    string tempOperand = operand;
    if (operand.substr(operand.length() - 2, 2) == ",X")
    {
      tempOperand = operand.substr(0, operand.length() - 2);
      xbpe = 8;
    }

    if (SYMTAB[tempOperand].exists == 'n')
    {
      
      if (halfBytes == 3)
      { 

        writeData = "Line " + to_string(lineNumber);
        
        writeData += " : Symbol doesn't exists. Found " + tempOperand;
      
        write_To_File(errorFile, writeData);
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
        objcode += (halfBytes == 5) ? (int_ToString_Hex(xbpe + 1, 1) + "00") : int_ToString_Hex(xbpe, 1);
        objcode += "000";
        return objcode;
      }

      
    }
    else
    {
      

      int operandAddress = string_Hex_To_Int(SYMTAB[tempOperand].address) + string_Hex_To_Int(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].startAddress);
      program_counter = address + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress);
      program_counter += (halfBytes == 5) ? 4 : 3;

      if (halfBytes == 3)
      {
        int relativeAddress = operandAddress - program_counter;
        if (relativeAddress >= (-2048) && relativeAddress <= 2047)
        {
          objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
          objcode += int_ToString_Hex(xbpe + 2, 1);
          objcode += int_ToString_Hex(relativeAddress, halfBytes);
          return objcode;
        }

        if (!nobase)
        {
          relativeAddress = operandAddress - base_register_value;
          if (relativeAddress >= 0 && relativeAddress <= 4095)
          {
            objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
            objcode += int_ToString_Hex(xbpe + 4, 1);
            objcode += int_ToString_Hex(relativeAddress, halfBytes);
            return objcode;
          }
        }

        if (operandAddress <= 4095)
        {
          objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
          objcode += int_ToString_Hex(xbpe, 1);
          objcode += int_ToString_Hex(operandAddress, halfBytes);

          modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
          modificationRecord += (halfBytes == 5) ? "05" : "03";
          modificationRecord += '\n';

          return objcode;
        }
      }
      else
      { // No base or pc based addressing in format 4
        objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
        objcode += int_ToString_Hex(xbpe + 1, 1);
        objcode += int_ToString_Hex(operandAddress, halfBytes);

        modificationRecord += "M^" + int_ToString_Hex(address + 1 + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
        modificationRecord += (halfBytes == 5) ? "05" : "03";
        modificationRecord += '\n';

        return objcode;
      }

      writeData = "Line: " + to_string(lineNumber);
      writeData += "Can not fit into program counter based or base register based addressing.";
      write_To_File(errorFile, writeData);
      objcode = int_ToString_Hex(string_Hex_To_Int(OPTAB[get_Real_Opcode(opcode)].opcode) + 3, 2);
      objcode += (halfBytes == 5) ? (int_ToString_Hex(xbpe + 1, 1) + "00") : int_ToString_Hex(xbpe, 1);
      objcode += "000";

      return objcode;
    }
  }
  return "AB";
}

void writeTextRecord(bool lastRecord = false)
{
  if (lastRecord)
  {
    if (currentRecord.length() > 0)
    { 
      writeData = int_ToString_Hex(currentRecord.length() / 2, 2) + '^' + currentRecord;
      write_To_File(objectFile, writeData);
      currentRecord = "";
    }
    return;
  }
  if (objectCode != "")
  {
    if (currentRecord.length() == 0)
    {
      writeData = "T^" + int_ToString_Hex(address + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      write_To_File(objectFile, writeData, false);
    }
    // If objectCode length > 60
    if ((currentRecord + objectCode).length() > 60)
    {
      // Current record
      writeData = int_ToString_Hex(currentRecord.length() / 2, 2) + '^' + currentRecord;
      write_To_File(objectFile, writeData);

      currentRecord = "";
      writeData = "T^" + int_ToString_Hex(address + string_Hex_To_Int(BLOCKS[BLocksNumToName[blockNumber]].startAddress), 6) + '^';
      write_To_File(objectFile, writeData, false);
    }

    currentRecord += objectCode;
  }
  else
  {
    //Assembler directive (doesn't result in address generation)
    if (opcode == "START" || opcode == "END" || opcode == "BASE" || opcode == "NOBASE" || opcode == "LTORG" || opcode == "ORG" || opcode == "EQU")
    {
    }
    else
    {
      if (currentRecord.length() > 0)
      {
        writeData = int_ToString_Hex(currentRecord.length() / 2, 2) + '^' + currentRecord;
        write_To_File(objectFile, writeData);
      }
      currentRecord = "";
    }
  }
}



void writeEndRecord(bool write = true)
{
  if (write)
  {
    if (endRecord.length() > 0)
    {
      write_To_File(objectFile, endRecord);
    }
    else
    {
      writeEndRecord(false);
    }
  }
  if ((operand == "" || operand == " ") && currentSectName == "DEFAULT")
  {
    endRecord = "E^" + int_ToString_Hex(startAddress, 6);
  }
  else if (currentSectName != "DEFAULT")
  {
    endRecord = "E";
  }
  else
  { 
    int firstExecutableAddress;

    firstExecutableAddress = string_Hex_To_Int(SYMTAB[firstExecutable_Sec].address);

    endRecord = "E^" + int_ToString_Hex(firstExecutableAddress, 6) + "\n";
  }
}

void pass2()
{
  string tempBuffer;
  intermediateFile.open("intermediate_" + fileName);
  if (!intermediateFile)
  {
    cout << "Unable to open: intermediate_" << fileName << endl;
    exit(1);
  }
  getline(intermediateFile, tempBuffer); 

  objectFile.open("object_" + fileName);
  if (!objectFile)
  {
    cout << "Unable to open: object_" << fileName << endl;
    exit(1);
  }

  ListingFile.open("listing_" + fileName);
  if (!ListingFile)
  {
    cout << "Unable to open: listing_" << fileName << endl;
    exit(1);
  }
  write_To_File(ListingFile, "Line\tAddress\tLabel\tOPCODE\tOPERAND\tObjectCode\tComment");

  errorFile.open("error_" + fileName, fstream::app);
  if (!errorFile)
  {
    cout << "Unable to open: error_" << fileName << endl;
    exit(1);
  }
  write_To_File(errorFile, "\nPASS2");

  objectCode = "";                                              //Global variables
  currentTextRecordLength = 0;
  currentRecord = "";
  modificationRecord = "";
  blockNumber = 0;
  nobase = true;

  readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  while (isComment)
  {
    writeData = to_string(lineNumber) + "\t" + comment;
    write_To_File(ListingFile, writeData);
    readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  }

  if (opcode == "START")
  {
    startAddress = address;
    writeData = to_string(lineNumber) + "\t" + int_ToString_Hex(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
    write_To_File(ListingFile, writeData);
  }
  else
  {
    label = "";
    startAddress = 0;
    address = 0;
  }
  
  program_section_length = program_length;


  writeData = "H^" + expand_String(label, 6, ' ', true) + '^' + int_ToString_Hex(address, 6) + '^' + int_ToString_Hex(program_section_length, 6);
  write_To_File(objectFile, writeData);

  readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
  currentTextRecordLength = 0;



  while (opcode != "END")
  {
    while (opcode != "END")
    {
      if (!isComment)
      {
        if (OPTAB[get_Real_Opcode(opcode)].exists == 'y')
        {
          if (OPTAB[get_Real_Opcode(opcode)].format == 1)
          {
            objectCode = OPTAB[get_Real_Opcode(opcode)].opcode;
          }
          else if (OPTAB[get_Real_Opcode(opcode)].format == 2)
          {
            operand1 = operand.substr(0, operand.find(','));
            operand2 = operand.substr(operand.find(',') + 1, operand.length() - operand.find(',') - 1);

            if (operand2 == operand)
            { // If not two operand i.e. a
              if (get_Real_Opcode(opcode) == "SVC")
              {
                objectCode = OPTAB[get_Real_Opcode(opcode)].opcode + int_ToString_Hex(stoi(operand1), 1) + '0';
              }
              else if (REGTAB[operand1].exists == 'y')
              {
                objectCode = OPTAB[get_Real_Opcode(opcode)].opcode + REGTAB[operand1].num + '0';
              }
              else
              {
                objectCode = get_Real_Opcode(opcode) + '0' + '0';
                writeData = "Line: " + to_string(lineNumber) + " Invalid Register name";
                write_To_File(errorFile, writeData);
              }
            }
            else
            { // Two operands i.e. a,b
              if (REGTAB[operand1].exists == 'n')
              {
                objectCode = OPTAB[get_Real_Opcode(opcode)].opcode + "00";
                writeData = "Line: " + to_string(lineNumber) + " Invalid Register name";
                write_To_File(errorFile, writeData);
              }
              else if (get_Real_Opcode(opcode) == "SHIFTR" || get_Real_Opcode(opcode) == "SHIFTL")
              {
                objectCode = OPTAB[get_Real_Opcode(opcode)].opcode + REGTAB[operand1].num + int_ToString_Hex(stoi(operand2), 1);
              }
              else if (REGTAB[operand2].exists == 'n')
              {
                objectCode = OPTAB[get_Real_Opcode(opcode)].opcode + "00";
                writeData = "Line: " + to_string(lineNumber) + " Invalid Register name";
                write_To_File(errorFile, writeData);
              }
              else
              {
                objectCode = OPTAB[get_Real_Opcode(opcode)].opcode + REGTAB[operand1].num + REGTAB[operand2].num;
              }
            }
          }
          else if (OPTAB[get_Real_Opcode(opcode)].format == 3)
          {
            if (get_Real_Opcode(opcode) == "RSUB")
            {
              objectCode = OPTAB[get_Real_Opcode(opcode)].opcode;
              objectCode += (get_Flag_Format(opcode) == '+') ? "000000" : "0000";
            }
            else
            {
              objectCode = createObjectCodeFormat34();
            }
          }
        } // If opcode in optab
        else if (opcode == "BYTE")
        {
          if (operand[0] == 'X')
          {
            objectCode = operand.substr(2, operand.length() - 3);
          }
          else if (operand[0] == 'C')
          {
            objectCode = string_To_Hex_String(operand.substr(2, operand.length() - 3));
          }
        }
        else if (label == "*")
        {
          if (opcode[1] == 'C')
          {
            objectCode = string_To_Hex_String(opcode.substr(3, opcode.length() - 4));
          }
          else if (opcode[1] == 'X')
          {
            objectCode = opcode.substr(3, opcode.length() - 4);
          }
        }
        else if (opcode == "WORD")
        {
          objectCode = int_ToString_Hex(stoi(operand), 6);
        }
        else if (opcode == "BASE")
        {
          if (SYMTAB[operand].exists == 'y')
          {
            base_register_value = string_Hex_To_Int(SYMTAB[operand].address) + string_Hex_To_Int(BLOCKS[BLocksNumToName[SYMTAB[operand].blockNumber]].startAddress);
            nobase = false;
          }
          else
          {
            writeData = "Line " + to_string(lineNumber) + " : Symbol doesn't exists. Found " + operand;
            write_To_File(errorFile, writeData);
          }
          objectCode = "";
        }
        else if (opcode == "NOBASE")
        {
          if (nobase)
          { // check whether base addressing or not
            writeData = "Line " + to_string(lineNumber) + ": Assembler wasn't using base addressing";
            write_To_File(errorFile, writeData);
          }
          else
          {
            nobase = true;
          }
          objectCode = "";
        }
        else
        {
          objectCode = "";
        }
        writeTextRecord();

        if (blockNumber == -1 && address != -1)
        {
          writeData = to_string(lineNumber) + "\t" + int_ToString_Hex(address) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
        }
        else if (address == -1)
        {
          
          writeData = to_string(lineNumber) + "\t" + " " + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
        }

        else
        {
          writeData = to_string(lineNumber) + "\t" + int_ToString_Hex(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
        }
      }
      else
      {
        writeData = to_string(lineNumber) + "\t" + comment;
      }
      write_To_File(ListingFile, writeData);                                                                             
      readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
      objectCode = "";
    } 
    writeTextRecord();

    writeEndRecord(false);
  }
    

    if (opcode != "CSECT")
    {
      while (readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment))
      {
        if (label == "*")
        {
          if (opcode[1] == 'C')
          {
            objectCode = string_To_Hex_String(opcode.substr(3, opcode.length() - 4));
          }
          else if (opcode[1] == 'X')
          {
            objectCode = opcode.substr(3, opcode.length() - 4);
          }
          writeTextRecord();
        }
        writeData = to_string(lineNumber) + "\t" + int_ToString_Hex(address) + "\t" + to_string(blockNumber) + label + "\t" + opcode + "\t" + operand + "\t" + objectCode + "\t" + comment;
        write_To_File(ListingFile, writeData);
      }
    }

    writeTextRecord(true);
    if (!isComment)
    {

      write_To_File(objectFile, modificationRecord, false);
      writeEndRecord(true);                           
      currentSectName = label;
      modificationRecord = "";
    }
    
   
    readIntermediateFile(intermediateFile, isComment, lineNumber, address, blockNumber, label, opcode, operand, comment);
    objectCode = "";
  }
  


int main()
{
  cout << "****Input file and executable(assembler.out) should be in same folder****" << endl
       << endl;
  cout << "Name of input file:";
  cin >> fileName;

  cout << "\nOPTAB Loading..." << endl;
  load_tables();

  cout << "\nPerforming PASS1..." << endl;
  cout << "Writing intermediate file to 'intermediate_" << fileName << "'" << endl;
  cout << "Writing error file to 'error_" << fileName << "'" << endl;
  pass1();

  cout << "Writing SYMBOL TABLE" << endl;
  printtab.open("tables_" + fileName);
  write_To_File(printtab, "SYMBOL TABLE-\n");
  for (auto const &it : SYMTAB)
  {
    writestring += it.first + ":-\t" + "Name:" + it.second.name + "\t|" + "Address:" + it.second.address + "\t|" + "Relative:" + int_ToString_Hex(it.second.relative) + " \n";
  }
  write_To_File(printtab, writestring);

  writestring = "";
  cout << "Writing LITERAL TABLE" << endl;

  write_To_File(printtab, "LITERAL TABLE-\n");
  for (auto const &it : LITTAB)
  {
    writestring += it.first + ":-\t" + "Value:" + it.second.value + "\t|" + "Address:" + it.second.address + " \n";
  }
  write_To_File(printtab, writestring);

 

  cout << "\nPerforming PASS2" << endl;
  cout << "Writing object file to 'object_" << fileName << "'" << endl;
  cout << "Writing listing file to 'listing_" << fileName << "'" << endl;
  pass2();
}
