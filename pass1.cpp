#include "utility.cpp"
#include "tables.cpp"

using namespace std;

string fileName;
bool error_flag = false;
int program_length;
string *BLocksNumToName;

string firstExecutable_Sec;

void handle_LTORG(string &litPrefix, int &lineNumberDelta, int lineNumber, int &LOCCTR, int &lastDeltaLOCCTR, int currentBlockNumber)
{
  string litAddress, litValue;
  litPrefix = "";
  for (auto const &it : LITTAB)
  {
    litAddress = it.second.address;
    litValue = it.second.value;
    if (litAddress != "?")
    {
    }
    else
    {
      lineNumber += 5;
      lineNumberDelta += 5;
      LITTAB[it.first].address = int_ToString_Hex(LOCCTR);
      LITTAB[it.first].blockNumber = currentBlockNumber;
      litPrefix += "\n" + to_string(lineNumber) + "\t" + int_ToString_Hex(LOCCTR) + "\t" + to_string(currentBlockNumber) + "\t" + "*" + "\t" + "=" + litValue + "\t" + " " + "\t" + " ";

      if (litValue[0] == 'X')
      {
        LOCCTR += (litValue.length() - 3) / 2;
        lastDeltaLOCCTR += (litValue.length() - 3) / 2;
      }
      else if (litValue[0] == 'C')
      {
        LOCCTR += litValue.length() - 3;
        lastDeltaLOCCTR += litValue.length() - 3;
      }
    }
  }
}

void evaluateExpression(string expression, bool &relative, string &tempOperand, int lineNumber, ofstream &errorFile, bool &error_flag)
{
  string singleOperand = "?", singleOperator = "?", valueString = "", valueTemp = "", writeData = "";
  int lastOperand = 0, lastOperator = 0, pairCount = 0;
  char lastByte = ' ';
  bool Illegal = false;

  for (int i = 0; i < expression.length();)
  {
    singleOperand = "";

    lastByte = expression[i];
    while ((lastByte != '+' && lastByte != '-' && lastByte != '/' && lastByte != '*') && i < expression.length())
    {
      singleOperand += lastByte;
      lastByte = expression[++i];
    }

    if (SYMTAB[singleOperand].exists == 'y')
    { // Checking if operand exists
      lastOperand = SYMTAB[singleOperand].relative;
      valueTemp = to_string(string_Hex_To_Int(SYMTAB[singleOperand].address));
    }
    else if ((singleOperand != "" || singleOperand != "?") && if_all_num(singleOperand))
    {
      lastOperand = 0;
      valueTemp = singleOperand;
    }
    else
    {
      writeData = "Line: " + to_string(lineNumber) + " : Symbol not found. Found " + singleOperand;
      write_To_File(errorFile, writeData);
      Illegal = true;
      break;
    }

    if (lastOperand * lastOperator == 1)
    { // Checking if expression is legal
      writeData = "Line: " + to_string(lineNumber) + " : Illegal expression found";
      write_To_File(errorFile, writeData);
      error_flag = true;
      Illegal = true;
      break;
    }
    else if ((singleOperator == "-" || singleOperator == "+" || singleOperator == "?") && lastOperand == 1)
    {
      if (singleOperator == "-")
      {
        pairCount--;
      }
      else
      {
        pairCount++;
      }
    }

    valueString += valueTemp;

    singleOperator = "";
    while (i < expression.length() && (lastByte == '+' || lastByte == '-' || lastByte == '/' || lastByte == '*'))
    {
      singleOperator += lastByte;
      lastByte = expression[++i];
    }

    if (singleOperator.length() > 1)
    {
      writeData = "Line: " + to_string(lineNumber) + " : Illegal operator. Found " + singleOperator;
      write_To_File(errorFile, writeData);
      error_flag = true;
      Illegal = true;
      break;
    }

    if (singleOperator == "*" || singleOperator == "/")
    {
      lastOperator = 1;
    }
    else
    {
      lastOperator = 0;
    }

    valueString += singleOperator;
  }

  if (!Illegal)
  {
    if (pairCount == 1)
    {

      relative = 1;
      Evaluate_String tempOBJ(valueString);
      tempOperand = int_ToString_Hex(tempOBJ.getResult());
    }
    else if (pairCount == 0)
    {

      relative = 0;
      cout << valueString << endl;
      Evaluate_String tempOBJ(valueString);
      tempOperand = int_ToString_Hex(tempOBJ.getResult());
    }
    else
    {
      writeData = "Line: " + to_string(lineNumber) + " : Illegal expression";
      write_To_File(errorFile, writeData);
      error_flag = true;
      tempOperand = "00000";
      relative = 0;
    }
  }
  else
  {
    tempOperand = "00000";
    error_flag = true;
    relative = 0;
  }
}
void pass1()
{
  ifstream sourceFile;
  ofstream intermediateFile, errorFile;

  sourceFile.open(fileName);
  if (!sourceFile)
  {
    cout << "Unable to open: " << fileName << endl;
    exit(1);
  }

  intermediateFile.open("intermediate_" + fileName);
  if (!intermediateFile)
  {
    cout << "Unable to open: intermediate_" << fileName << endl;
    exit(1);
  }
  write_To_File(intermediateFile, "Line\tAddress\tLabel\tOPCODE\tOPERAND\tComment");
  errorFile.open("error_" + fileName);
  if (!errorFile)
  {
    cout << "Unable to open error_" << fileName << endl;
    exit(1);
  }
  write_To_File(errorFile, "PASS1");

  string fileLine;
  string writeData, writeDataSuffix = "", writeDataPrefix = "";
  int index = 0;

  string currentBlockName = "DEFAULT";
  int currentBlockNumber = 0;
  int totalBlocks = 1;

  bool statusCode;
  string label, opcode, operand, comment;
  string tempOperand;

  int startAddress, LOCCTR, saveLOCCTR, lineNumber, lastDeltaLOCCTR, lineNumberDelta = 0;
  lineNumber = 0;
  lastDeltaLOCCTR = 0;

  getline(sourceFile, fileLine);
  lineNumber += 5;
  while (check_Comment_Line(fileLine))
  {
    writeData = to_string(lineNumber) + "\t" + fileLine;
    write_To_File(intermediateFile, writeData);
    getline(sourceFile, fileLine);
    lineNumber += 5;
    index = 0;
  }

  read_First_Non_White_Space(fileLine, index, statusCode, label);
  read_First_Non_White_Space(fileLine, index, statusCode, opcode);

  if (opcode == "START")
  {
    read_First_Non_White_Space(fileLine, index, statusCode, operand);
    read_First_Non_White_Space(fileLine, index, statusCode, comment, true);
    startAddress = string_Hex_To_Int(operand);

    LOCCTR = startAddress;
    writeData = to_string(lineNumber) + "\t" + int_ToString_Hex(LOCCTR - lastDeltaLOCCTR) + "\t" + to_string(currentBlockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment;
    write_To_File(intermediateFile, writeData);

    getline(sourceFile, fileLine);
    lineNumber += 5;
    index = 0;
    read_First_Non_White_Space(fileLine, index, statusCode, label);
    read_First_Non_White_Space(fileLine, index, statusCode, opcode);
  }
  else
  {
    startAddress = 0;
    LOCCTR = 0;
  }

  string currentSectName = "DEFAULT";
  int sectionCounter = 0;

  while (opcode != "END")
  {

    while (opcode != "END" && opcode != "CSECT")
    {

      if (!check_Comment_Line(fileLine))
      {
        if (label != "")
        { // Label found
          if (SYMTAB[label].exists == 'n')
          {
            SYMTAB[label].name = label;
            SYMTAB[label].address = int_ToString_Hex(LOCCTR);
            SYMTAB[label].relative = 1;
            SYMTAB[label].exists = 'y';
            SYMTAB[label].blockNumber = currentBlockNumber;
          }
          else
          {
            writeData = "Line: " + to_string(lineNumber) + " : Duplicate symbol for '" + label + "'. Previously defined at " + SYMTAB[label].address;
            write_To_File(errorFile, writeData);
            error_flag = true;
          }
        }
        if (OPTAB[get_Real_Opcode(opcode)].exists == 'y')
        { // Search for opcode in OPTAB
          if (OPTAB[get_Real_Opcode(opcode)].format == 3)
          {
            LOCCTR += 3;
            lastDeltaLOCCTR += 3;
            if (get_Flag_Format(opcode) == '+')
            {
              LOCCTR += 1;
              lastDeltaLOCCTR += 1;
            }
            if (get_Real_Opcode(opcode) == "RSUB")
            {
              operand = " ";
            }
            else
            {
              read_First_Non_White_Space(fileLine, index, statusCode, operand);
              if (operand[operand.length() - 1] == ',')
              {
                read_First_Non_White_Space(fileLine, index, statusCode, tempOperand);
                operand += tempOperand;
              }
            }

            if (get_Flag_Format(operand) == '=')
            {
              tempOperand = operand.substr(1, operand.length() - 1);
              if (tempOperand == "*")
              {
                tempOperand = "X'" + int_ToString_Hex(LOCCTR - lastDeltaLOCCTR, 6) + "'";
              }
              if (LITTAB[tempOperand].exists == 'n')
              {
                LITTAB[tempOperand].value = tempOperand;
                LITTAB[tempOperand].exists = 'y';
                LITTAB[tempOperand].address = "?";
                LITTAB[tempOperand].blockNumber = -1;
              }
            }
          }
          else if (OPTAB[get_Real_Opcode(opcode)].format == 1)
          {
            operand = " ";
            LOCCTR += OPTAB[get_Real_Opcode(opcode)].format;
            lastDeltaLOCCTR += OPTAB[get_Real_Opcode(opcode)].format;
          }
          else
          {
            LOCCTR += OPTAB[get_Real_Opcode(opcode)].format;
            lastDeltaLOCCTR += OPTAB[get_Real_Opcode(opcode)].format;
            read_First_Non_White_Space(fileLine, index, statusCode, operand);
            if (operand[operand.length() - 1] == ',')
            {
              read_First_Non_White_Space(fileLine, index, statusCode, tempOperand);
              operand += tempOperand;
            }
          }
        }

        else if (opcode == "WORD")
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);
          LOCCTR += 3;
          lastDeltaLOCCTR += 3;
        }
        else if (opcode == "RESW")
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);
          LOCCTR += 3 * stoi(operand);
          lastDeltaLOCCTR += 3 * stoi(operand);
        }
        else if (opcode == "RESB")
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);
          LOCCTR += stoi(operand);
          lastDeltaLOCCTR += stoi(operand);
        }
        else if (opcode == "BYTE")
        {
          read_Byte_Operand(fileLine, index, statusCode, operand);
          if (operand[0] == 'X')
          {
            LOCCTR += (operand.length() - 3) / 2;
            lastDeltaLOCCTR += (operand.length() - 3) / 2;
          }
          else if (operand[0] == 'C')
          {
            LOCCTR += operand.length() - 3;
            lastDeltaLOCCTR += operand.length() - 3;
          }
        }
        else if (opcode == "BASE")
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);
        }
        else if (opcode == "LTORG")
        {
          operand = " ";
          handle_LTORG(writeDataSuffix, lineNumberDelta, lineNumber, LOCCTR, lastDeltaLOCCTR, currentBlockNumber);
        }
        else if (opcode == "ORG")
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);

          char lastByte = operand[operand.length() - 1];
          while (lastByte == '+' || lastByte == '-' || lastByte == '/' || lastByte == '*')
          {
            read_First_Non_White_Space(fileLine, index, statusCode, tempOperand);
            operand += tempOperand;
            lastByte = operand[operand.length() - 1];
          }

          int tempVariable;
          tempVariable = saveLOCCTR;
          saveLOCCTR = LOCCTR;
          LOCCTR = tempVariable;

          if (SYMTAB[operand].exists == 'y')
          {
            LOCCTR = string_Hex_To_Int(SYMTAB[operand].address);
          }
          else
          {
            bool relative;
            error_flag = false;
            evaluateExpression(operand, relative, tempOperand, lineNumber, errorFile, error_flag);
            if (!error_flag)
            {
              LOCCTR = string_Hex_To_Int(tempOperand);
            }
            error_flag = false; 
          }
        }
        else if (opcode == "USE")
        {

          read_First_Non_White_Space(fileLine, index, statusCode, operand);
          BLOCKS[currentBlockName].LOCCTR = int_ToString_Hex(LOCCTR);

          if (BLOCKS[operand].exists == 'n')
          {

            BLOCKS[operand].exists = 'y';
            BLOCKS[operand].name = operand;
            BLOCKS[operand].number = totalBlocks++;
            BLOCKS[operand].LOCCTR = "0";
          }

          currentBlockNumber = BLOCKS[operand].number;
          currentBlockName = BLOCKS[operand].name;
          LOCCTR = string_Hex_To_Int(BLOCKS[operand].LOCCTR);
        }
        else if (opcode == "EQU")
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);
          tempOperand = "";
          bool relative;

          if (operand == "*")
          {
            tempOperand = int_ToString_Hex(LOCCTR - lastDeltaLOCCTR, 6);
            relative = 1;
          }
          else if (if_all_num(operand))
          {
            tempOperand = int_ToString_Hex(stoi(operand), 6);
            relative = 0;
          }
          else
          {
            char lastByte = operand[operand.length() - 1];

            while (lastByte == '+' || lastByte == '-' || lastByte == '/' || lastByte == '*')
            {
              read_First_Non_White_Space(fileLine, index, statusCode, tempOperand);
              operand += tempOperand;
              lastByte = operand[operand.length() - 1];
            }

            // Read whole operand
            evaluateExpression(operand, relative, tempOperand, lineNumber, errorFile, error_flag);
          }

          SYMTAB[label].name = label;
          SYMTAB[label].address = tempOperand;
          SYMTAB[label].relative = relative;
          SYMTAB[label].blockNumber = currentBlockNumber;
          lastDeltaLOCCTR = LOCCTR - string_Hex_To_Int(tempOperand);
        }
        else
        {
          read_First_Non_White_Space(fileLine, index, statusCode, operand);
          writeData = "Line: " + to_string(lineNumber) + " : Invalid OPCODE present. Found " + opcode;
          write_To_File(errorFile, writeData);
          error_flag = true;
        }
        read_First_Non_White_Space(fileLine, index, statusCode, comment, true);
        if (opcode == "EQU" && SYMTAB[label].relative == 0)
        {
          writeData = writeDataPrefix + to_string(lineNumber) + "\t" + int_ToString_Hex(LOCCTR - lastDeltaLOCCTR) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;
        }

        else
        {
          writeData = writeDataPrefix + to_string(lineNumber) + "\t" + int_ToString_Hex(LOCCTR - lastDeltaLOCCTR) + "\t" + to_string(currentBlockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;
        }
        writeDataPrefix = "";
        writeDataSuffix = "";
      }
      else
      {
        writeData = to_string(lineNumber) + "\t" + fileLine;
      }
      write_To_File(intermediateFile, writeData);

      BLOCKS[currentBlockName].LOCCTR = int_ToString_Hex(LOCCTR); // Update LOCCTR of block after every instruction
      getline(sourceFile, fileLine);
      lineNumberDelta = 0;
      index = 0;
      lastDeltaLOCCTR = 0;
      read_First_Non_White_Space(fileLine, index, statusCode, label);  // Parsing label
      read_First_Non_White_Space(fileLine, index, statusCode, opcode); // Parsing OPCODE
    }

    if (opcode != "END")
    {

      if (SYMTAB[label].exists == 'n')
      {
        SYMTAB[label].name = label;
        SYMTAB[label].address = int_ToString_Hex(LOCCTR);
        SYMTAB[label].relative = 1;
        SYMTAB[label].exists = 'y';
        SYMTAB[label].blockNumber = currentBlockNumber;
      }

      write_To_File(intermediateFile, writeDataPrefix + to_string(lineNumber) + "\t" + int_ToString_Hex(LOCCTR - lastDeltaLOCCTR) + "\t" + to_string(currentBlockNumber) + "\t" + label + "\t" + opcode);

      getline(sourceFile, fileLine);
      lineNumber += 5;

      read_First_Non_White_Space(fileLine, index, statusCode, label);  // Parsing label
      read_First_Non_White_Space(fileLine, index, statusCode, opcode); // Parsing OPCODE
    }
  }

  if (opcode == "END")
  {
    firstExecutable_Sec = SYMTAB[label].address;
    SYMTAB[firstExecutable_Sec].name = label;
    SYMTAB[firstExecutable_Sec].address = firstExecutable_Sec;
  }

  read_First_Non_White_Space(fileLine, index, statusCode, operand);
  read_First_Non_White_Space(fileLine, index, statusCode, comment, true);

  currentBlockName = "DEFAULT";
  currentBlockNumber = 0;
  LOCCTR = string_Hex_To_Int(BLOCKS[currentBlockName].LOCCTR);

  handle_LTORG(writeDataSuffix, lineNumberDelta, lineNumber, LOCCTR, lastDeltaLOCCTR, currentBlockNumber);

  writeData = to_string(lineNumber) + "\t" + int_ToString_Hex(LOCCTR - lastDeltaLOCCTR) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + comment + writeDataSuffix;
  write_To_File(intermediateFile, writeData);

  int LocctrArr[totalBlocks];
  BLocksNumToName = new string[totalBlocks];
  for (auto const &it : BLOCKS)
  {
    LocctrArr[it.second.number] = string_Hex_To_Int(it.second.LOCCTR);
    BLocksNumToName[it.second.number] = it.first;
  }

  for (int i = 1; i < totalBlocks; i++)
  {
    LocctrArr[i] += LocctrArr[i - 1];
  }

  for (auto const &it : BLOCKS)
  {
    if (it.second.startAddress == "?")
    {
      BLOCKS[it.first].startAddress = int_ToString_Hex(LocctrArr[it.second.number - 1]);
    }
  }

  program_length = LocctrArr[totalBlocks - 1] - startAddress;

  sourceFile.close();
  intermediateFile.close();
  errorFile.close();
}
