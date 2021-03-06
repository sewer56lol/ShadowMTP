/*
	HeroesBLK, my third C++ program, utility to decompress Shadow The Hedgehog's
	Motion Packages .MTP, archives into raw non-Renderware .anm(s).
    Copyright (C) 2017  Sewer56lol

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

// basic file operations
#include <iostream> //I/O Stream
#include <iomanip> //I/O Manipulation
#include <fstream> //File Stream
#include <string> // String
#include <cstring> //Strings
#include <sstream> //String Stream
#include <vector> //Vector the crocodiles

#include <dirent.h> // Directory Entities

// Speedup: Use this on Linux! Search for #DANKMEME.
//#include "byteswap.h" //Woot, mah endians m8

using namespace std; //Standard Namespace

// Search OLDBroken for OLD code for previous struct

class ShadowANMObject
{
	public:
		// Default Constructor, choochoo!
		ShadowANMObject() {}

		// Animation Name, obtained from the Animation Name Offset
		string AnimationName;

		// Animation Size, obtained from the Animation Offset
		unsigned int AnimationDataSize;

		// Animation Property Size, obtained from the AnimationPropertyOffset offset, up until the next AnimationPropertyOffset offset.
		unsigned int AnimationPropertySize = 0;

		/* --------------------------------------------------------------- */
		// OFFSETS

		// Offset where the animation size will be contained. (at 0x8 offset)
		unsigned int AnimationDataOffset;

		// Offset where the animation
		unsigned int AnimationNameOffset;

		// Unknown Value
		unsigned int AnimationPropertyOffset;

		/* --------------------------------------------------------------- */
		// ONLY FOR REPACK

		// Animation Property name, this is a file name, only used for repacking! (2nd Struct Section of Name Tables!)
		string AnimationPropertyName;

		// Null bytes for 4 byte alignment performed in the format, null. Used only for repacking! (2nd Struct Section of Name Tables!)
		unsigned short int AnimationNameNullBytes = 0;

		// Absolute Animation Name Length for repacking
		unsigned short int AnimationNameLength = 0;

		// Animation Name raw
		string AnimationNameRaw;
};

bool DoesFileExist(string fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

int16_t LittleToBigEndian16(int16_t val)
{
    return (val << 8) |          // left-shift always fills with zeros
          ((val >> 8) & 0x00ff); // right-shift sign-extends, so force to zero
}

int32_t LittleToBigEndian32(int32_t val)
{
    return (val << 24) |
          ((val <<  8) & 0x00ff0000) |
          ((val >>  8) & 0x0000ff00) |
          ((val >> 24) & 0x000000ff);
}

unsigned short int GetStringNullBytes(string StringMeme)
{
	if (StringMeme.length() == 0) { return 0; }
	unsigned short int NullByteCount = 0;
	unsigned short int StringLength = StringMeme.length();

	for (unsigned int x = 0; x < 4; x++)
	{
		unsigned short NotDivisibleByFourInitial = StringLength % 4;
		unsigned short NotDivisibleByFourNew = (StringLength + NullByteCount) % 4; 
	    if (NotDivisibleByFourInitial == 0 && x == 0) // Even if divisible by 4, MTP pads it with 4 bytes anyway.
		{
			NullByteCount += 1;
		}
		else if ( NotDivisibleByFourNew != 0) // If not divisible by 4, add 1.
		{
			NullByteCount += 1;
		}
		else { break; }
	}
	return NullByteCount;
}

/* Literally name of method, as on the tin */
bool StringHasEnding (string const &fullString, string const &ending) 
{
    if (fullString.length() >= ending.length()) 
	{
       	return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } 
	else 
	{
        return false;
    }
}

void MTPToANM(string InputFile, string OutputFile) {

	// File size
	size_t FileSize = 0;

	// A copy of the file will be stored here.
	char* MTPFileData;

	// Just for storing individual bytes, makes my head feel more comfortable.
	char* TwoByteBuffer; TwoByteBuffer = new char[2];
	char* FourByteBuffer; FourByteBuffer = new char[4];
	char* EightByteBuffer; EightByteBuffer = new char[8];

	// Amount of individual stored animations.
	unsigned short int AnimationEntries;

	// Make the directory specified
	const char* OutputDirectory = OutputFile.c_str();
	mkdir(OutputDirectory);

	// Load the file.
	ifstream FileReader(InputFile,ios::binary);

	// Load file and get file size ==>
	// Validates if reading of the file has been successful.
	if (! FileReader) { cout << "Error opening file" << endl; } else { cout << "Successfully opened object file\n" << endl; }
	FileReader.seekg(0, ios::end); // Set the pointer to the end
	FileSize = FileReader.tellg(); // Get the pointer location
	FileReader.seekg(0, ios::beg); // Set the pointer to the beginning

	// Make a vector to store all of the animation names, offsets and sizes.
	vector<ShadowANMObject> ShadowObjects;

	// Allocates enough room for the MTP file.
	MTPFileData = new char[FileSize + 1];

	// Read the file into the array.
	FileReader.read(MTPFileData, FileSize);

	// This will get us our animation entry amount #.
	TwoByteBuffer[0] = MTPFileData[3]; TwoByteBuffer[1] = MTPFileData[2];

	// Set the Animation Entries to the found hex value;
	AnimationEntries = *(unsigned short *)TwoByteBuffer;

	// Resize the vector to equal the amount of animation entries in file;
	// ShadowObjects.resize(AnimationEntries);

	// Print ObjectHeader
	cout << "--------------" << endl;
	cout << "Animation Data" << endl;
	cout << "--------------\n" << endl;

	// Print AnimCount to console.
	cout << "Shadow MotionPackage Animations Count: " << AnimationEntries << endl;

	// Start the CurrentAnimationPointerLocation at 0x10; Where Animation Entries begin.
	unsigned int CurrentAnimationLocation = 8;

	// OLDBROKEN: unsigned int CurrentAnimationLocation = 0x10;

	// For each animation entry in .MTP
	// This is the main information extraction loop.

	// OLDBROKEN: Used to be AnimationEntries + 0, but file header skimps on 1st prop.
	for(unsigned int x = 0; x < AnimationEntries + 1;)
	{
		cout << "\n =>> Animation: " << x << endl;
		ShadowANMObject* TemporaryShadowObject = new ShadowANMObject();

		// Note: We're working with Little Endian systems, load the data backwards.

		/* Get the Unknown Value */
		FourByteBuffer[0] = MTPFileData[CurrentAnimationLocation + 3];
		FourByteBuffer[1] = MTPFileData[CurrentAnimationLocation + 2];
		FourByteBuffer[2] = MTPFileData[CurrentAnimationLocation + 1];
		FourByteBuffer[3] = MTPFileData[CurrentAnimationLocation + 0];
		TemporaryShadowObject -> AnimationNameOffset = *(unsigned int *)FourByteBuffer; // OLDBROKEN: TemporaryShadowObject -> AnimationPropertyOffset = *(unsigned int *)FourByteBuffer;
		

		/* Get the Animation Name Offset */
		FourByteBuffer[0] = MTPFileData[CurrentAnimationLocation + 7];
		FourByteBuffer[1] = MTPFileData[CurrentAnimationLocation + 6];
		FourByteBuffer[2] = MTPFileData[CurrentAnimationLocation + 5];
		FourByteBuffer[3] = MTPFileData[CurrentAnimationLocation + 4];
		TemporaryShadowObject -> AnimationDataOffset = *(unsigned int *)FourByteBuffer; // OLDBROKEN: TemporaryShadowObject -> AnimationNameOffset = *(unsigned int *)FourByteBuffer

		/* Get the Animation Data Offset */
		FourByteBuffer[0] = MTPFileData[CurrentAnimationLocation + 11];
		FourByteBuffer[1] = MTPFileData[CurrentAnimationLocation + 10];
		FourByteBuffer[2] = MTPFileData[CurrentAnimationLocation + 9];
		FourByteBuffer[3] = MTPFileData[CurrentAnimationLocation + 8];
		TemporaryShadowObject -> AnimationPropertyOffset = *(unsigned int *)FourByteBuffer; // OLDBROKEN: TemporaryShadowObject -> AnimationDataOffset = *(unsigned int *)FourByteBuffer

		/* Print Details of Object */
		cout << "Animation Name Offset: " << TemporaryShadowObject -> AnimationNameOffset << endl;
		cout << "Animation Data Offset: " << TemporaryShadowObject -> AnimationDataOffset << endl;
		cout << "Archive Property Offset: " << TemporaryShadowObject -> AnimationPropertyOffset << endl;

		/* Resolve Data Size Of File */
		/*
			Note, we are grabbing it from the .anm file itself at offset 0x8 inside the file.
			This data starts at AnimationDataOffset.
		*/

		/* Get data size offset from inside the .anm file */
		unsigned int AnimationDataSizeOffset = (TemporaryShadowObject -> AnimationDataOffset) + 0x8;

		/* Dump this data size to the object */
		FourByteBuffer[0] = MTPFileData[AnimationDataSizeOffset + 3];
		FourByteBuffer[1] = MTPFileData[AnimationDataSizeOffset + 2];
		FourByteBuffer[2] = MTPFileData[AnimationDataSizeOffset + 1];
		FourByteBuffer[3] = MTPFileData[AnimationDataSizeOffset + 0];
		TemporaryShadowObject -> AnimationDataSize = *(unsigned int *)FourByteBuffer;

		cout << "Animation Size: " << TemporaryShadowObject -> AnimationDataSize << endl;

		/* Resolve Object Name */
		/*
			Reads hex string at the offset until meeting \x00.
			Merely appends to a vector.
		*/
		unsigned int AnimationNameOffset = TemporaryShadowObject -> AnimationNameOffset;
		unsigned int StringCharacter = 0; // Current character in string.
		string ObjectNameRaw;
		bool Delimiter = false;

		// Get characters until \x00 is hit.
		while (Delimiter == false)
		{
			// If not null.
			if ( MTPFileData[AnimationNameOffset + StringCharacter] != 0x00 )
			{
				ObjectNameRaw += (char)MTPFileData[AnimationNameOffset + StringCharacter];
			}
			else
			{
				Delimiter = true;
			}

			// Go to next char.
			StringCharacter++;
		}

		TemporaryShadowObject -> AnimationName = ObjectNameRaw + ".STHAnim";
		cout << "Animation Name: " << TemporaryShadowObject -> AnimationName << endl;

		// Add the new object data to the list of objects in vector.
		// Must point to pointer of this object.
		ShadowObjects.push_back( *(ShadowANMObject *)TemporaryShadowObject );
		// Seek to next animation's base address.
		CurrentAnimationLocation += 0xC;
		x++; // Let's go, to the next one, my friend.
	}

	/*
		Alright, we have all the info we need, on every object, now let's extract the objects :3
	*/

	cout << "\n--------------\nWriting Files:\n" << endl;

	/* 
		The two lines below exist in order to write a text file which will contain, in order all of the extracted files, 
		while unconfirmed, my look and analysis suggests that when repacking, it will be a good idea to ensure that the first
		two animations of the archive will be repacked in identical order at the beginning, as they appear to have different lengths
		of properties. This may not be necessary but I'm implementing this as a failsafe.'		
	*/
	string TextFilePath = OutputFile + "/" + "ShadowMTPDec.txt"; // Store path for text file to be written.
	ofstream ANMDecTextFile; ANMDecTextFile.open(TextFilePath); // Generates a text file for writing.

	/* Print a modified version of the above message */
	ANMDecTextFile << "This file has been automatically generated by ShadowMTP.\n\nThe lines present below this paragraph list the files in order\nof which they have been extracted from the original archive,\nwhile unconfirmed, my look and analysis suggests that when repacking,\nit will be a good idea to ensure that at least the first\ntwo animations featuring the unknown property section should probably\nbest be repacked in identical order at the beginning as seen in the archive\nas they appear to have different lengths of properties than the rest.\n\nIf you wish to pack your own additional files, just add the filename into the list below.\nThis may not be necessary but I'm implementing this as a failsafe.\n" << endl;

	/* These variables will be used to seek ahead through objects to determine the data unknown property length */
	unsigned int ObjectLoopCounter = 1; // Used to enumerate loop counts, is always 1 ahead of the current vector entry.
	unsigned int VectorLength = ShadowObjects.size(); // Gets length of the vector, no of entries.
	unsigned int PropertyStartOffset = ShadowObjects[VectorLength - 1].AnimationDataOffset + ShadowObjects[VectorLength - 1].AnimationDataSize; // Where the property section starts.

	/* Set 1st to correct offset (now being 1st property) */ // OLDBROKEN
	/* 
		ShadowObjects[0].AnimationPropertyOffset = PropertyStartOffset;
		cout << "Animation Name for 1st Property (Above list starts from last): " << ShadowObjects[0].AnimationName << endl;
		cout << "New Offset: " << ShadowObjects[0].AnimationPropertyOffset << endl;
	*/

	/* Fix Animation Property Offsets For Writing */
	/* This is done by shifting all of the offsets down, i.e. next is previous, except for 1st */ // OLDBROKEN
	/*
		for (unsigned int x = 1; x < VectorLength;) // Ignore 1st entry
		{
			ShadowObjects[x].AnimationPropertyOffset = ShadowObjects[x + 1].AnimationPropertyOffset; // Shift down by 1
			x++;
		}
	*/
	
	/* Removes first invalid entry after shift */
	/* 	
		for (unsigned int x = 1; x < VectorLength;) // Ignore 1st entry // OLDBROKEN
		{
			if (ShadowObjects[x].AnimationPropertyOffset != 0)
			{
				ShadowObjects[x].AnimationPropertyOffset = 0; // Remove false entry without property post shift.
				break;
			}
			x++;
		}
	*/
		
	/* Calculate lengths of object properties */
	for(unsigned int x = 0; x <= VectorLength;)
	{
		// If the offset is not 0
		if (ShadowObjects[x].AnimationPropertyOffset != 0)
		{
			// Seek from next item
			for(unsigned int y = x + 1; y <= VectorLength;)
			{

				if (ShadowObjects[y].AnimationPropertyOffset != 0) // If non null offset
				{
					unsigned int TMP1 = ShadowObjects[y].AnimationPropertyOffset;
					unsigned int TMP2 = ShadowObjects[x].AnimationPropertyOffset;
					unsigned int Delta = TMP1 - TMP2;

					// Size = difference between this and next offset.
					ShadowObjects[x].AnimationPropertySize = Delta;
					//cout << "AnimPropName: " << ShadowObjects[x].AnimationPropertyName << endl;
					//cout << "AnimPropSize: " << ShadowObjects[x].AnimationPropertySize << endl;
					break;
				}

				y++;		
			}

			// Break from this loop if found.
		}
		x++;
	}

	// Set the length of the last object to be from offset till end
	ShadowObjects[VectorLength - 1].AnimationPropertySize = FileSize - ShadowObjects[VectorLength - 1].AnimationPropertyOffset;  

	// Seek start address, right after last mapped object.
	//unsigned int SeekStartAddress = ShadowObjects[VectorLength - 3].AnimationPropertyOffset + ShadowObjects[VectorLength - 3].AnimationPropertySize;
	/* Manually seek for last two objects */
	// OLDBROKEN
	/*
	for (unsigned int x = VectorLength - 2; x < VectorLength;) // For last 2 objects || -2 because zero index.
	{

		unsigned int ObjectLength = 1; // 
		// For all of the remaining bytes //
		for (unsigned int y = SeekStartAddress; y <= FileSize + 1;)
		{
			unsigned int DelimiterTest1 = (unsigned char)MTPFileData[y];
			unsigned int DelimiterTest2 = (unsigned char)MTPFileData[y-1];

			// Go up incrementing bytes until FF FF
			if ( DelimiterTest1 == 0xFF && DelimiterTest2 == 0xFF )
			{
				ShadowObjects[x].AnimationPropertySize = ObjectLength; // Declare new length for current object.
				ShadowObjects[x+1].AnimationPropertyOffset = ShadowObjects[x].AnimationPropertyOffset + ShadowObjects[x].AnimationPropertySize; // Set offset of next to one of previous + length. 
				break;
			}
			else 
			{
				ObjectLength += 1;
			}
			y++; // Go to next byte!
		}

		x++; // Go to next object!
	}
	*/
	

	/* This section is responsible for writing the animation files! */
	/* I shouldn't declare a for loop like this when I already have a value for size and an index, but oh well.. */
	for (auto &ShadowANMObject : ShadowObjects)
	{
		// DEBUG //
		string TemporaryFilePath = OutputFile + "/" + ShadowANMObject.AnimationName;
		cout << TemporaryFilePath << endl;

		// Write File Name to Text File //
		ANMDecTextFile << ShadowANMObject.AnimationName << endl;

		// Creates an outputstream for the ANM file
		ofstream ANMFile;
		// Opens the outputstream for binary writing
		ANMFile.open(TemporaryFilePath,ios::binary);

		/* If the file is successfully opened and created */
		if (ANMFile.is_open())
		{
			// Gets our first byte address for file.
			unsigned int DataOffset = ShadowANMObject.AnimationDataOffset;
			// Gets our last byte address for file.
			unsigned int DataOffsetEnd = DataOffset + ShadowANMObject.AnimationDataSize;

			/* Copy relevant data byte by byte */
			/*
				There's probably a better, faster way to copy the data range, I'm just worried
				about preserving Big Endian though, as I'm still relatively new to C++.
			*/
			for (unsigned int x = DataOffset; x < DataOffsetEnd;)
			{
				// Push the current byte to file.
				ANMFile << MTPFileData[x];
				// Add a byte to loop.
				x++;
			}
		}
		else { cout << "Couldn't generate/write new file, do you have write permissions in your chosen directory?" << endl; }

		ANMFile.close();
	}

	// Add a line between files and properties.
	ANMDecTextFile << endl;

	/* Fix | Swap Animation Property Length and Property Offset for 1st Value */
	unsigned int TempSizeProperty = ShadowObjects[0].AnimationPropertyOffset;
	ShadowObjects[0].AnimationPropertyOffset = ShadowObjects[0].AnimationPropertySize;
	ShadowObjects[0].AnimationPropertySize = TempSizeProperty; // This "object" does not have a size, it is a pointer to the table of properties, I'm only treating it as one so it doesn't feel lonely.
															   // In order to not rewrite code, I'll dump the offset of this as bytes of equal length.

	/* I know it's literally the same loop, and I'm wasting cycles, it's just that I'd rather keep Properties separate for readability, and for writing the properties files, and the text file in desired order, and I'm lazy */
	for (auto &ShadowANMObject : ShadowObjects)
	{
		// If there is a property, i.e. size not null.
		if (ShadowANMObject.AnimationPropertySize != 0)
		{
			// DEBUG //
			string TemporaryFilePath = OutputFile + "/" + ShadowANMObject.AnimationName + "Property";
			cout << "Offset: " << ShadowANMObject.AnimationPropertyOffset << " || " << "Length: " << ShadowANMObject.AnimationPropertySize << " || " << TemporaryFilePath << endl;

			// Creates an outputstream for the ANMProperty file
			ofstream ANMFile;
			// Opens the outputstream for binary writing
			ANMFile.open(TemporaryFilePath,ios::binary);

			/* If the file is successfully opened and created */
			if (ANMFile.is_open())
			{
				// Gets our first byte address for file.
				unsigned int DataOffset = ShadowANMObject.AnimationPropertyOffset;
				// Gets our last byte address for file.
				unsigned int DataOffsetEnd = DataOffset + ShadowANMObject.AnimationPropertySize;

				/* Copy relevant data byte by byte */
				/*
					There's probably a better, faster way to copy the data range, I'm just worried
					about preserving Big Endian though, as I'm still relatively new to C++.
				*/
				for (unsigned int x = DataOffset; x < DataOffsetEnd;)
				{
					// Push the current byte to file.
					ANMFile << MTPFileData[x];
					// Add a byte to loop.
					x++;
				}
			}
			else { cout << "Couldn't generate/write new file, do you have write permissions in your chosen directory?" << endl; }

			/* Done with the new file. */
			ANMFile.close();

			/* We should write a filename for the property in the file */
			ANMDecTextFile << ShadowANMObject.AnimationName << "Property" << endl;
		}
	}

	ANMDecTextFile.close(); // Close.
}


// --------------------

void FolderToMTP(string InputFile, string OutputFile)
{
	
	// File to be written.
	vector<char> MTPFileData;
	// Objects to be written.
	vector<ShadowANMObject> ShadowObjects;
	// Input File TXT.
	string TXTArchivePropertiesFile = InputFile + "/ShadowMTPDec.txt";
	// Input file stream to read the individual files.
	ifstream FileReader; 

	// Just for storing individual bytes, makes my head feel more comfortable.
	char* TwoByteBuffer; TwoByteBuffer = new char[2];
	char* FourByteBuffer; FourByteBuffer = new char[4];
	char* EightByteBuffer; EightByteBuffer = new char[8];

	// Variables for reading the definition of text file.
	string CurrentLine;		// Current Line of Text file.
	unsigned short int AnimationEntries = 0; // Incremented when a file with .STHAnim is found.
	unsigned int AnimationProperties = 0; // Incremented when a file with .STHAnimProperty is found.
	vector<string> ObjectNameStrings; // Stores the strings of all of the object names.
	vector<string> ObjectPropertyStrings; // Stores the strings of all of the object properties.

	// First we are going to calculate the amount of entries for animations we are going to have.
	FileReader.open(TXTArchivePropertiesFile);
	while( getline(FileReader,CurrentLine) )
	{
		// If string ends correctly
		if ( StringHasEnding(CurrentLine , ".STHAnim") )
		{
			// Create temporary empty object.
			ShadowANMObject* TemporaryObject = new ShadowANMObject();
			// Push the string of the name of the current object off to the vector of strings.
			ObjectNameStrings.push_back(CurrentLine);
			// Push the temporary object into the vector.
			ShadowObjects.push_back( *(ShadowANMObject *)TemporaryObject );
			// Add an entry to the complete total of animation entries.
			AnimationEntries = AnimationEntries + 1;
		}
		else if ( StringHasEnding(CurrentLine , ".STHAnimProperty") )
		{ 
			// => Do not repeat object creation here, a property cannot exist without an anim.
			// => Anim count will already be set by STHAnim files.
			ObjectPropertyStrings.push_back(CurrentLine);
		}
	}
	FileReader.close();

	// Iterate through all objects, add the file object properties to the appropriate object.
	for (unsigned int x = 0; x < ShadowObjects.size(); x++)
	{		
		// Set Name of Animation //
		ShadowObjects[x].AnimationName = ObjectNameStrings[x];
		
		// Open File Temporarily //
		FileReader.open(InputFile + "/" + ShadowObjects[x].AnimationName, ios::binary); // Open the file
		FileReader.seekg(0, ios::end); // Set the pointer to the end
		unsigned int TempFileSize = FileReader.tellg(); // Get the pointer location
		
		ShadowObjects[x].AnimationDataSize = TempFileSize; // Set the data size of the file.
		FileReader.close(); // Closé

		for (unsigned int y = 0; y < ObjectPropertyStrings.size(); y++)
		{
			string StripProperty = ObjectPropertyStrings[y]; // Declare Property Name String
			StripProperty =  StripProperty.substr(0, StripProperty.size() - 8); // Strip "Property from Name"
			
			if (ShadowObjects[x].AnimationName == StripProperty) // If property animation name matches animation name, add it to the object.
			{
				// Open Property File Temporarily //
				FileReader.open(InputFile + "/" + ObjectPropertyStrings[y], ios::binary); // Open the file
				FileReader.seekg(0, ios::end); // Set the pointer to the end
				unsigned int TempPropertyFileSize = FileReader.tellg(); // Get pointer location
				ShadowObjects[x].AnimationPropertyName = ObjectPropertyStrings[y]; // Pass the file name into the object.
				ShadowObjects[x].AnimationPropertySize = TempPropertyFileSize; // Set the property size
				FileReader.close(); // Close the file
			}
		}
		cout << "Property Name: " << ShadowObjects[x].AnimationName << " || " << ShadowObjects[x].AnimationPropertySize << " || " << ShadowObjects[x].AnimationDataSize << endl;
	}

	/* Writing to the MTP File */

	// Writer to new MTP file.
	ofstream FileWriter(OutputFile, ios::binary);
	// Check if file successfully written.
	if (FileWriter.is_open()) { cout << "File successfully created for writing.\n" << endl; } else { cout << "File failed to open for writing" << endl; std::exit; }
	// Activator for header
	//AnimationEntries += 1; // Part of the hacky implementation, see below! OLDBROKEN
	unsigned const short MTPArchiveActivator = 0x01;
	unsigned const short MTPArchiveActivatorBigEndian = LittleToBigEndian16(MTPArchiveActivator);
	unsigned const short AnimationInfoEntryOffset = 12;
	unsigned const short HeaderNullBytes = 0x00;
	unsigned const short AnimationInfoTableStart = 8;
	unsigned int NameSectionStart = 0;
	unsigned int DataSectionStart = 0;
	unsigned int PropertySectionStart = 0;

	/* Calculate the File Format Offsets Before Writing */

	/* Starting off With a HACKY Implementation to fix a HACKY implementation, god bless! */
	// This is now all OLDBROKEN.... HAHAHHAHAHAHHAHAHAHHAHAHAHAHAHAHAHAHAHAHAHAH
	/*
		ShadowANMObject* TemporaryObjectX = new ShadowANMObject();
		TemporaryObjectX->AnimationPropertySize = ShadowObjects[0].AnimationPropertySize;
		TemporaryObjectX->AnimationPropertyName = ShadowObjects[0].AnimationPropertyName;
		ShadowObjects[0].AnimationPropertySize = 0;
		ShadowObjects[0].AnimationPropertyName = "";
		TemporaryObjectX->AnimationDataOffset = 0;
		TemporaryObjectX->AnimationNameOffset = 0;
		TemporaryObjectX->AnimationDataSize = 0;
		ShadowObjects.insert( ShadowObjects.begin(), *(ShadowANMObject *)TemporaryObjectX );
	*/
	
	/* We can start calculating the offsets now */
	NameSectionStart = (AnimationInfoEntryOffset * AnimationEntries) + AnimationInfoTableStart;

	cout << "Calculating Data Offsets: " << endl;
	cout << "---------------------------" << endl;
	
	// Let's get the 4 byte alignment of nulls the file names first though!
	for (unsigned int x = 0; x < AnimationEntries; x++)
	{
		// Animation Name Offset ✔
		// Animation Size 
		//

		cout << "---------" << endl;
		cout << "Object: " << x << endl;
		cout << "---------" << endl;
		ShadowObjects[x].AnimationNameNullBytes = GetStringNullBytes( ShadowObjects[x].AnimationName.substr(0, ShadowObjects[x].AnimationName.size() - 8) );
		cout << "Object Name: " << ShadowObjects[x].AnimationName.substr(0, ShadowObjects[x].AnimationName.size() - 8) << endl;
		cout << "Object Null Bytes: " << ShadowObjects[x].AnimationNameNullBytes << endl;


		/* Calculating Name Lengths! */
		if ( ShadowObjects[x].AnimationName.length() < 9) { } // Do Nothing if the animation name length is null
		else 
		{ 
			ShadowObjects[x].AnimationNameRaw = ShadowObjects[x].AnimationName.substr(0, ShadowObjects[x].AnimationName.size() - 8); // Set raw anim name,
			ShadowObjects[x].AnimationNameLength = ShadowObjects[x].AnimationName.length() - 8; // Do NOT put this below in the else statement, things'll break!
		} 

		/* Calculating Name Offsets! */
		if (x == 0) { ShadowObjects[x].AnimationNameOffset = NameSectionStart; }
													// Last string's offset                      // Length of name				// Remove extension 			// Add null bytes
		else { ShadowObjects[x].AnimationNameOffset = ShadowObjects[x - 1].AnimationNameOffset +  ShadowObjects[x - 1].AnimationNameLength + ShadowObjects[x - 1].AnimationNameNullBytes; }
	}

	DataSectionStart = ShadowObjects[AnimationEntries - 1].AnimationNameOffset + ShadowObjects[AnimationEntries - 1].AnimationNameLength + ShadowObjects[AnimationEntries - 1].AnimationNameNullBytes;

	/* Calculating Data Offsets! */
	for (unsigned int x = 0; x < AnimationEntries; x++)
	{
		
		if (x == 0) { ShadowObjects[x].AnimationDataOffset = DataSectionStart; }
													
		else { ShadowObjects[x].AnimationDataOffset = ShadowObjects[x - 1].AnimationDataOffset + ShadowObjects[x - 1].AnimationDataSize; }
	}

	PropertySectionStart = ShadowObjects[AnimationEntries - 1].AnimationDataOffset + ShadowObjects[AnimationEntries - 1].AnimationDataSize;

	/* Calculating Property Offsets! */
	for (unsigned int x = 0; x < AnimationEntries; x++)
	{
		if (x == 1) { ShadowObjects[x].AnimationPropertyOffset = PropertySectionStart; }
													
		else { ShadowObjects[x].AnimationPropertyOffset = ShadowObjects[x - 1].AnimationPropertyOffset + ShadowObjects[x - 1].AnimationPropertySize; }
	}

	/* Wiping offsets where length == 0 */
	/* We do not allocate space in table to null offsets */
	for (unsigned int x = 0; x < AnimationEntries; x++)
	{
		if (ShadowObjects[x].AnimationDataSize == 0) { ShadowObjects[x].AnimationDataOffset = 0; }
		if (ShadowObjects[x].AnimationNameLength == 0) { ShadowObjects[x].AnimationNameOffset = 0; }
		if (ShadowObjects[x].AnimationPropertySize == 0) { ShadowObjects[x].AnimationPropertyOffset = 0; }		
	}

	cout << endl;
	cout << "Displaying To Be Packed File Data!" << endl;
	cout << "----------------------------------" << endl;

	for (unsigned int x = 0; x < ShadowObjects.size(); x++)
	{
		cout << "Object: " << x << endl;
		cout << "----------" << endl;
		cout << "File Name: " << ShadowObjects[x].AnimationName << endl;
		cout << "File Name Raw: " << ShadowObjects[x].AnimationNameRaw << endl;
		cout << "Property Name: " << ShadowObjects[x].AnimationPropertyName << endl;
		cout << "File Name Offset: " << ShadowObjects[x].AnimationNameOffset << endl;
		cout << "File Name Length: " << ShadowObjects[x].AnimationNameLength << endl;
		cout << "File Name Null Bytes: " << ShadowObjects[x].AnimationNameNullBytes << endl;
		cout << "File Data Offset: " << ShadowObjects[x].AnimationDataOffset << endl;
		cout << "File Data Size: " << ShadowObjects[x].AnimationDataSize << endl;
		cout << "File Property Offset: " << ShadowObjects[x].AnimationPropertyOffset << endl;
		cout << "File Property Size: " << ShadowObjects[x].AnimationPropertySize << endl;
		cout << endl;
	}

	cout << "Packing!" << endl;
	cout << "--------" << endl;

	cout << "\n=> Writing File Header!" << endl;
	// Time to write the file header //
	unsigned short AnimationEntriesBigEndian = LittleToBigEndian16(AnimationEntries - 1); 
	
	// The archive format doesn't treat the first entry as an actual animation, despite having data offset for its property.
	FileWriter.write( (char*)&MTPArchiveActivatorBigEndian, 2 ); // Activator 1 
	FileWriter.write( (char*)&AnimationEntriesBigEndian, 2 ); // Animation Entries
	FileWriter.write( (char*)&MTPArchiveActivatorBigEndian, 2 ); // Activator 2
	FileWriter.write( (char*)&HeaderNullBytes, 1 ); // Null x1
	FileWriter.write( (char*)&HeaderNullBytes, 1 ); // Null x2

	cout << "=> Writing File Entries In Table!" << endl;
	// Write the file entries to the table //
	ShadowObjects[0].AnimationPropertyOffset = ShadowObjects[0].AnimationPropertySize; // One final fix, first entry uses size instead of offset for the property offset, IDK why.
	for (unsigned short x = 0; x < AnimationEntries; x++)
	{
		unsigned int AnimationNameOffsetBigEndian = LittleToBigEndian32(ShadowObjects[x].AnimationNameOffset);
		unsigned int AnimationDataOffsetBigEndian = LittleToBigEndian32(ShadowObjects[x].AnimationDataOffset);
		unsigned int AnimationPropertyOffsetBigEndian = LittleToBigEndian32(ShadowObjects[x].AnimationPropertyOffset);
		FileWriter.write( (char*)&AnimationNameOffsetBigEndian, 4);
		FileWriter.write( (char*)&AnimationDataOffsetBigEndian, 4);
		FileWriter.write( (char*)&AnimationPropertyOffsetBigEndian, 4);
	}
	

	cout << "=> Writing The Filename Array!" << endl;
	// Write the filename array // 
	for (unsigned short x = 0; x < AnimationEntries; x++)
	{
		unsigned short NullBytes = ShadowObjects[x].AnimationNameNullBytes;
		FileWriter << ShadowObjects[x].AnimationNameRaw;
		//FileWriter.write( (char*)&ShadowObjects[x].AnimationNameRaw, ShadowObjects[x].AnimationNameRaw.length());
		for (unsigned short x = 0; x < NullBytes; x++) 
		{ 
			FileWriter.write( (char*)&HeaderNullBytes, 1); 
		} 
	}

	cout << "=> Writing The File Data!" << endl;
	// Write the data array. //
	for (unsigned short x = 0; x < AnimationEntries; x++)
	{
		char* TemporaryDataArray; TemporaryDataArray = new char[ShadowObjects[x].AnimationDataSize];
		unsigned int AnimationDataSizeTemp = ShadowObjects[x].AnimationDataSize;
		FileReader.open(InputFile + "/" + ShadowObjects[x].AnimationName, ios::binary); // Open the animation file.
		FileReader.read(TemporaryDataArray, AnimationDataSizeTemp); // Read the data into the array

		for (unsigned int x = 0; x < AnimationDataSizeTemp; x++)
		{
			// Push the current byte to file.
			FileWriter << TemporaryDataArray[x];
		}
		FileReader.close();
	}

	cout << "=> Writing The Property Array!" << endl;
	// Write the property array //
	for (unsigned short x = 0; x < AnimationEntries; x++)
	{
		string FileString = InputFile + "/" + ShadowObjects[x].AnimationPropertyName;
		// This is the table pointer, the table pointer contains no data, because it is so similar, I am simply not writing the property data so I can treat is as an object here, it doesn't contain any actual data, even if dumped.
		if (x == 0) { } 
		else if ( DoesFileExist(FileString) == 0 ) {} // Do nothing if file does not exist.
		else
		{
			char* TemporaryDataArray; TemporaryDataArray = new char[ShadowObjects[x].AnimationPropertySize];
			unsigned int AnimationDataSizeTemp = ShadowObjects[x].AnimationPropertySize;
			FileReader.open(InputFile + "/" + ShadowObjects[x].AnimationPropertyName, ios::binary); // Open the animation file.
			FileReader.read(TemporaryDataArray, AnimationDataSizeTemp); // Read the data into the array

			for (unsigned int y = 0; y < AnimationDataSizeTemp; y++)
			{
				// Push the current byte to file.
				FileWriter << TemporaryDataArray[y];
			}
			FileReader.close();
		}
		
	}

}

// --------------------

int main(int argc, char ** argv)
{
	string InputFile; //Input file
	string OutputFile; //Output file
	int Action = 0; // Extract or compile?

	// Identify the command line passed arguments.
	for(int i = 1; i < argc; i++)
	{
		if (strcmp (argv[i], "--extract") == 0) { Action = 1; }
		if (strcmp (argv[i], "--compile") == 0) { Action = 2; }
		if (strcmp (argv[i], "-i") == 0) { InputFile = argv[i+1]; cout << "Input File/Folder: " << InputFile << endl; }
		if (strcmp (argv[i], "-o") == 0) { OutputFile = argv[i+1]; cout << "Output File/Folder: " << OutputFile << endl; }
	}

	if (Action == 1) { MTPToANM(InputFile, OutputFile); }
	else if (Action == 2) { FolderToMTP(InputFile, OutputFile); } // Do Nothing
	else
	{
		cout << "\n\nYou have not specified an action. Try running with parameters:" << endl;
		cout << "--extract -i <MTPFile> -o <MTPFolder>" << endl;
		cout << "--compile -i <MTPFolder> -o <MTPFile>" << endl;
	}
	return 0;
}
