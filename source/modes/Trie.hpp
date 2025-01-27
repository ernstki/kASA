/***************************************************************************
*  Part of kASA: https://github.com/SilvioWeging/kASA
*
*  Copyright (C) 2020 Silvio Weging <silvio.weging@gmail.com>
*
*  Distributed under the Boost Software License, Version 1.0.
*  (See accompanying file LICENSE_1_0.txt or copy at
*  http://www.boost.org/LICENSE_1_0.txt)
**************************************************************************/
#pragma once

#include "../MetaHeader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace TrieSnG { // Statics and globals used inside this header
	const uint8_t _iNumOfAminoAcids = 32; // convenience, better than looking through the list everytime

	///DEBUG
	//const uint8_t LISTOFAA[32] = { '@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','[','\\', ']', '^', '_' };

	///

	//static uint64_t iNumOfNodes[5] = { 0,0,0,0,0 };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Leaf5 {
	// Sizeof(Leaf): 372 Byte
	uint64_t _vRanges1[TrieSnG::_iNumOfAminoAcids];
	uint32_t _vRanges2[TrieSnG::_iNumOfAminoAcids];


	Leaf5() {
		for (uint8_t i = 0; i < TrieSnG::_iNumOfAminoAcids; ++i) {
			_vRanges1[i] = numeric_limits<uint64_t>::max();
			_vRanges2[i] = 0ul;
		}
	}

	inline uint64_t GetFirst() {
		for (uint8_t i = 0; i < TrieSnG::_iNumOfAminoAcids; ++i) {
			if (_vRanges1[i] != numeric_limits<uint64_t>::max()) {
				return _vRanges1[i];
			}
		}
		return numeric_limits<uint64_t>::max();
	}

	inline uint64_t GetLast() {
		for (int8_t i = static_cast<int8_t>(TrieSnG::_iNumOfAminoAcids) - 1; i >= 0; --i) {
			if (_vRanges1[i] != numeric_limits<uint64_t>::max()) {
				return _vRanges1[i] + _vRanges2[i];
			}
		}
		return 0;
	}

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A Node from a Graph(Trie) with a list of its Edges and current level
class Node {
private:

	// Sizeof(Node): 256 Byte
	void* _Edges[TrieSnG::_iNumOfAminoAcids];

	int8_t _iLevelDiff;

public:

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void IncreaseIndex(const uint32_t& kMer, const uint64_t& iStartIdx, const uint32_t& iEndIdx, uint64_t& iSizeOfTrie) {
		if (_iLevelDiff - 2 > 0) {
			const uint8_t& tempVal = (uint8_t)((kMer >> (_iLevelDiff - 1) * 5) & 31); // read only the relevant 5 bytes (and start with level 1)
			//cout << bitset<15>(tempVal) << endl;
			//cout << TrieSnG::LISTOFAA[tempVal] << " " << kASA::kASA::kMerToAminoacid(kMer, 12) << endl;

			Node* edge = static_cast<Node*>(_Edges[tempVal]);
			if (edge == nullptr) {
				//iNumOfNodes[_iLevel] += 1;
				_Edges[tempVal] = new Node(kMer, _iLevelDiff - 1, iStartIdx, iEndIdx, iSizeOfTrie);
				iSizeOfTrie += 256;
			}
			else {
				edge->IncreaseIndex(kMer, iStartIdx, iEndIdx, iSizeOfTrie);
			}
		}
		else {
			const uint8_t& tempVal = (uint8_t)((kMer >> (_iLevelDiff - 1) * 5) & 31);
			const uint8_t& lastVal = (uint8_t)((kMer >> (_iLevelDiff - 2) * 5) & 31);

			//cout << TrieSnG::LISTOFAA[tempVal] << " " << TrieSnG::LISTOFAA[lastVal]  << " " << kASA::kASA::kMerToAminoacid(kMer,12) << endl;

			Leaf5* edge = static_cast<Leaf5*>(_Edges[tempVal]);
			if (edge == nullptr) {
				_Edges[tempVal] = new Leaf5();
				iSizeOfTrie += sizeof(Leaf5);
				static_cast<Leaf5*>(_Edges[tempVal])->_vRanges1[lastVal] = iStartIdx;
				static_cast<Leaf5*>(_Edges[tempVal])->_vRanges2[lastVal] = iEndIdx;
			}
			else {
				edge->_vRanges1[lastVal] = iStartIdx;
				edge->_vRanges2[lastVal] = iEndIdx;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void InOrderTraversal(const int8_t& iCurrentK, const int8_t& iMaxLevel, const vector<uint64_t>& vIn, vector<uint64_t>& vOut, uint64_t& currIdx) {
		if (_iLevelDiff - 2 > 0) {
			for (uint32_t iLetterIdx = 0; iLetterIdx < 32; ++iLetterIdx) {
				Node* edge = static_cast<Node*>(_Edges[iLetterIdx]);
				if (edge != nullptr) {
					edge->InOrderTraversal(iCurrentK, iMaxLevel, vIn, vOut, currIdx);
				}
			}
		}
		else {
			for (uint32_t iLetterIdx = 0; iLetterIdx < 32; ++iLetterIdx) {
				Leaf5* edge = static_cast<Leaf5*>(_Edges[iLetterIdx]);
				if (edge != nullptr) {
					for (uint32_t iLastIdx = 0; iLastIdx < 32; ++iLastIdx) {
						if (edge->_vRanges1[iLastIdx] != numeric_limits<uint64_t>::max()) {
							for (uint32_t iNumOfIdenticals = 0; iNumOfIdenticals < edge->_vRanges2[iLastIdx]; ++iNumOfIdenticals) {
								vOut[currIdx] = vIn[edge->_vRanges1[iLastIdx]];
								currIdx++;
							}
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	explicit Node(const int8_t& iMaxLevel) : _Edges(), _iLevelDiff(iMaxLevel) {
		// for root node
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Node(const uint32_t& kMer, const int8_t& iLevel, const uint64_t& iStartIdx, const uint32_t& iEndIdx, uint64_t& iSizeOfTrie) : _Edges(), _iLevelDiff(iLevel) {
		IncreaseIndex(kMer, iStartIdx, iEndIdx, iSizeOfTrie);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	~Node() {
		if (_iLevelDiff - 2 > 0) {
			for (uint8_t i = 0; i < TrieSnG::_iNumOfAminoAcids; ++i) {
				delete static_cast<Node*>(_Edges[i]);
			}
		}
		else {
			for (uint8_t i = 0; i < TrieSnG::_iNumOfAminoAcids; ++i) {
				delete static_cast<Leaf5*>(_Edges[i]);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline void GetRange(const uint64_t& kMer, const int8_t& iCurrentK, const int8_t& iMinK, const int8_t& iMaxLevel, uint64_t&& start, uint32_t&& range) {
		/// First handle the usual case
		if (_iLevelDiff != iMaxLevel - iCurrentK) {
			if (_iLevelDiff - 2 > 0) {
				const uint8_t& tempVal = (uint8_t)((kMer >> (_iLevelDiff - 1) * 5) & 31); // read only the relevant 5 bytes (and start with level 1)
				//cout << TrieSnG::LISTOFAA[tempVal] << endl;
				Node* edge = static_cast<Node*>(_Edges[tempVal]);
				if (edge == nullptr) {
					start = numeric_limits<uint64_t>::max();
					return;
				}
				else {
					edge->GetRange(kMer, iCurrentK, iMinK, iMaxLevel, move(start), move(range));
				}
			}
			else {
				const uint8_t& tempVal = (uint8_t)((kMer >> (_iLevelDiff - 1) * 5) & 31);
				//cout << TrieSnG::LISTOFAA[tempVal] << endl;
				Leaf5* edge = static_cast<Leaf5*>(_Edges[tempVal]);
				if (edge == nullptr) {
					start = numeric_limits<uint64_t>::max();
					return;
				}
				else {
					const uint8_t& lastVal = (uint8_t)((kMer >> (_iLevelDiff - 2) * 5) & 31);
					//cout << TrieSnG::LISTOFAA[lastVal] << endl;
					start = edge->_vRanges1[lastVal];
					range = edge->_vRanges2[lastVal];
				}
			}
		}
		else {
			start = GetRangeAfterMinK(iMaxLevel, true);
			range = static_cast<uint32_t>(GetRangeAfterMinK(iMaxLevel, false) - start);
			return;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline bool isInTrie(const uint32_t& kMer, const int8_t& iMinK, const int8_t& iMaxLevel) const {
		/// First handle the usual case
		if (_iLevelDiff != iMaxLevel - iMinK) {
			if (_iLevelDiff - 2 > 0) {
				const uint8_t& tempVal = (uint8_t)((kMer >> (5 + _iLevelDiff - iMaxLevel) * 5) & 31); // read only the relevant 5 bytes (and start with level 1)
				
				//cout << TrieSnG::LISTOFAA[tempVal] << endl;
				
				Node* edge = static_cast<Node*>(_Edges[tempVal]);
				if (edge == nullptr) {
					return false;
				}
				else {
					return edge->isInTrie(kMer, iMinK, iMaxLevel);
				}
			}
			else {
				const uint8_t& tempVal = (uint8_t)((kMer >> (5 + _iLevelDiff - iMaxLevel) * 5) & 31);

				//cout << TrieSnG::LISTOFAA[tempVal] << endl;

				Leaf5* edge = static_cast<Leaf5*>(_Edges[tempVal]);
				if (edge == nullptr) {
					return false;
				}
				else {
					const uint8_t& lastVal = (uint8_t)((kMer >> (5 + _iLevelDiff - iMaxLevel - 1) * 5) & 31);

					//cout << TrieSnG::LISTOFAA[lastVal] << endl;

					return (edge->_vRanges1[lastVal] != numeric_limits<uint64_t>::max());
				}
			}
		}
		else {
			return true;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline uint64_t GetRangeAfterMinK(const int8_t& iMaxLevel, const bool& bStart) {
		if (_iLevelDiff - 2 > 0) {
			if (bStart) {
				for (uint8_t i = 0; i < TrieSnG::_iNumOfAminoAcids; ++i) {
					if (_Edges[i]) {
						return static_cast<Node*>(_Edges[i])->GetRangeAfterMinK(iMaxLevel, true);
					}
				}
				return 0;
			}
			else {
				for (int8_t i = static_cast<int8_t>(TrieSnG::_iNumOfAminoAcids) - 1; i >= 0; --i) {
					if (_Edges[i]) {
						return static_cast<Node*>(_Edges[i])->GetRangeAfterMinK(iMaxLevel, false);
					}
				}
				return numeric_limits<uint64_t>::max();
			}
			
		}
		else {
			if (bStart) {
				for (uint8_t i = 0; i < TrieSnG::_iNumOfAminoAcids; ++i) {
					if (_Edges[i]) {
						return static_cast<Leaf5*>(_Edges[i])->GetFirst();
					}
				}
				return 0;
			}
			else {
				for (int8_t i = static_cast<int8_t>(TrieSnG::_iNumOfAminoAcids) - 1; i >= 0; --i) {
					if (_Edges[i]) {
						return static_cast<Leaf5*>(_Edges[i])->GetLast();
					}
				}
			}
			return numeric_limits<uint64_t>::max();
		}
	}


};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Prefix-tree for a logarithmic access of the first few letters
template<class intType>
class Trie {
private:
	
	int8_t _iMaxK, _iMinK, _iMaxLevel, _ikForIsInTrie;
	intType _Bitmask = 31;

	// this counts the size of the trie in bytes to substract it later from the available memory in RAM
	uint64_t iSizeOfTrie = 0;

	unique_ptr<Node> _root;

	unique_ptr<packedBigPair[]> _prefixLookuptable;
	const uint64_t pow30Array[6] = {1,30,900,27000,810000,24300000};
	vector<tuple<uint32_t,uint64_t,uint32_t>> _prefixArray;

public:
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Trie(const int8_t& iMaxK, const int8_t& iMinK, const int8_t& iMaxLevel, const uint8_t& iPrefixCheckMode = 0) : _iMaxK(iMaxK), _iMinK(iMinK), _iMaxLevel(iMaxLevel), _ikForIsInTrie(6) {
		for (uint8_t i = 1; i < iMaxLevel; ++i) {
			_Bitmask |= 31ULL << (5 * i);
		}
		_Bitmask <<= (iMaxK - iMaxLevel) * 5;

		switch (iPrefixCheckMode) {
		case 1:
			// use trie
			break;
		case 2:
			_prefixArray.resize(1);
			break;
		case 3:
#if (__aarch64__ || _M_ARM)
			break;
#else
			_prefixLookuptable.reset(new packedBigPair[754137931]);
			fill(_prefixLookuptable.get(), _prefixLookuptable.get() + 754137931, packedBigPair(numeric_limits<uint64_t>::max(), 0)); // 754137930 = 30+30*30+30*30^2+30*30^3+30*30^4+30*30^5 = "]^^^^^" as integer without spaces
			iSizeOfTrie = 754137931 * sizeof(packedBigPair);
			break;
#endif
		default:
			break;
		};

		_root.reset(new Node(_iMaxLevel));
	}

	inline void addToTrie(const uint32_t& entry, const uint64_t& iIdx, const uint32_t& iValue) {
		uint64_t iDummy = 0;
		_root->IncreaseIndex(entry, iIdx, iValue, iDummy);
	}

	inline void SetForIsInTrie(const uint8_t& val) {
		_ikForIsInTrie = val;
	}

	inline uint64_t GetSize() {
		return iSizeOfTrie;
	}

	inline void GetIfVecIsUsed() {
		if (_prefixLookuptable) {
			cout << "OUT: Lookup table (faster) will be used for prefix matching..." << endl;
		}
		else {
			if (_prefixArray.size()) {
				cout << "OUT: Array will be used for prefix matching..." << endl;
			}
			else {
				cout << "OUT: Trie will be used for prefix matching..." << endl;
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Save to file
	template<typename T>
	inline void SaveToStxxlVec(const T* vKMerVec, const string& savePath) { //passing a non-const reference with default value is a hassle
		try {
			Utilities::checkIfFileCanBeCreated(savePath + "_trie");
			stxxlFile trieFile(savePath+"_trie", stxxl::file::RDWR);
			trieVector trieVec(&trieFile, 0);
			uint64_t iCount = 1;
			auto iKnownShortMer = vKMerVec->at(0).first & _Bitmask;
			stxxl::vector_bufreader<typename T::const_iterator> bufferedReader(vKMerVec->cbegin() + 1, vKMerVec->cend(), 0);
			for (; !bufferedReader.empty(); ++bufferedReader) {
				const auto& entry = *bufferedReader;
				const auto& iTemp = entry.first & _Bitmask;
				if (iTemp != iKnownShortMer) {
					trieVec.push_back(packedBigPairTrie(uint32_t(iKnownShortMer >> (_iMaxK - _iMaxLevel) * 5), iCount));
					iKnownShortMer = iTemp;
					iCount = 1;
				}
				else {
					++iCount;
				}
			}
			trieVec.push_back(packedBigPairTrie(uint32_t(iKnownShortMer >> (_iMaxK - _iMaxLevel) * 5), iCount));
			ofstream sizeFile(savePath + "_trie.txt");
			sizeFile << trieVec.size();
			trieVec.export_files("_");
		}
		catch (...) {
			cerr << "ERROR: in: " << __PRETTY_FUNCTION__ << endl; throw;
		}
	}


	///////////////////////////
	inline void LoadFromStxxlVec(const string& loadPath) {
		try {
			ios::sync_with_stdio(false);
			ifstream sizeFile(loadPath + "_trie.txt");
			uint64_t iSizeOfTrieVec;
			sizeFile >> iSizeOfTrieVec;
			if (_prefixArray.size()) {
				_prefixArray.reserve(iSizeOfTrieVec);
			}
			stxxlFile trieFile(loadPath + "_trie", stxxl::file::RDONLY);
			const trieVector trieVec(&trieFile, iSizeOfTrieVec);
			uint64_t iStart = 0, iEnd = 0, iCount = 0;
			uint32_t iReducedkMer;

			//ofstream smallAA("D:/tmp/paper/trie.txt");
			//uint64_t iMaxRange = 0, iSumOfRanges = 0, iNumOfRanges = 0;
			//vector<uint32_t> vRanges;
			for (const auto& entry : trieVec) {
				iReducedkMer = entry.first;
				iCount = entry.second;
				//vRanges.push_back(iCount);
				//if (iCount > iMaxRange) {
				//	iMaxRange = iCount;
				//}
				//iSumOfRanges += iCount;
				//++iNumOfRanges;
				//cout << kASA::kASA::kMerToAminoacid(iReducedkMer, 6) << " " << iCount << endl;
				const uint64_t& iSum = iCount + iEnd;
				const int32_t iDiff = static_cast<int32_t>(int64_t(iSum) - 1 - iStart);

				assert(iDiff >= 0);
				if (_prefixLookuptable) {
					int64_t iIndexInTable = 0;
					for (int8_t i = 0; i < 6; ++i) {
						iIndexInTable += (iReducedkMer & 31) * pow30Array[i];
						iReducedkMer >>= 5;
					}
					_prefixLookuptable[iIndexInTable] = packedBigPair(iStart, iDiff);
				}
				else {
					if (_prefixArray.size() > 0) {
						_prefixArray.push_back(make_tuple(iReducedkMer, iStart, iDiff));
					}
					else {
						_root->IncreaseIndex(iReducedkMer, iStart, static_cast<uint32_t>(iDiff), iSizeOfTrie);
					}
				}
				iStart = iSum;
				iEnd += iCount;
			}


			if (_prefixArray.size() > 0) {
				iSizeOfTrie = iSizeOfTrieVec * sizeof(tuple<uint32_t, uint64_t, uint32_t>);
			}

			//sort(vRanges.begin(), vRanges.end());
			//cout << vRanges[vRanges.size() / 2] << " " << vRanges.back() << endl;
			//cout << iMaxRange << " " << iSumOfRanges/double(iNumOfRanges) << " " << iNumOfRanges <<  endl;
			//cout << "Nodes per Level: 1:" << iNumOfNodes[0] << " 2:" << iNumOfNodes[1] << " 3:" << iNumOfNodes[2] << " 4:" << iNumOfNodes[3] << " 5:" << iNumOfNodes[4] << endl;
		}
		catch (...) {
			cerr << "ERROR: in: " << __PRETTY_FUNCTION__ << endl; throw;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create without saving
	template<typename T>
	inline void Create(const T vKMerVec) { //TODO!!!

		uint64_t iCount = 0, iStart = 0; //iEnd = 0;
		uint64_t iKnownShortMer = get<0>(vKMerVec->at(0)) & _Bitmask;
		uint64_t iTemp = 0;
		for (const auto& elem : *vKMerVec) {
			const auto& entry = get<0>(elem);
			//cout << kASA::kASA::kMerToAminoacid(entry, 12) << endl;
			iTemp = entry & _Bitmask;
			if (iTemp != iKnownShortMer) {
				const uint64_t& iSum = iCount + iStart;
				_root->IncreaseIndex(static_cast<uint32_t>(iKnownShortMer >> (_iMaxK - _iMaxLevel) * 5), iStart, static_cast<uint32_t>( iSum - 1), iSizeOfTrie);
				//cout << kASA::kASA::kMerToAminoacid(entry, 12) << " " << kASA::kASA::kMerToAminoacid((iKnownShortMer >> (_iMaxK - _iMaxLevel) * 5), 12) << endl;
				//iEnd += iCount;
				iKnownShortMer = iTemp;
				iStart = iSum;
				iCount = 1;
			}
			else {
				++iCount;
			}
		}
		_root->IncreaseIndex(static_cast<uint32_t>(iTemp >> (_iMaxK - _iMaxLevel) * 5), iStart, static_cast<uint32_t>(iCount + iStart - 1), iSizeOfTrie);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Access
	inline void GetIndexRange(const uint64_t& kMer, const int8_t& iCurrentK, uint64_t&& start, uint32_t&& range) const {
		if (_prefixLookuptable) {
			int64_t iIndexInTable = 0;
			uint64_t kMerCopy = kMer;
			for (int8_t i = 0; i < 6; ++i) {
				iIndexInTable += (kMerCopy & 31) * pow30Array[i];;
				kMerCopy >>= 5;
			}
			start = _prefixLookuptable[iIndexInTable].first;
			range = _prefixLookuptable[iIndexInTable].second;
		}
		else {
			if (_prefixArray.size() > 0) {
				auto res = lower_bound(_prefixArray.cbegin(), _prefixArray.cend(), kMer, [](const tuple<uint32_t, uint64_t, uint32_t>& a, const uint64_t& val) { return get<0>(a) < val; });
				if (res != _prefixArray.cend() && get<0>(*res) == kMer) {
					start = get<1>(*res);
					range = get<2>(*res);
				}
				else {
					start = numeric_limits<uint64_t>::max();
				}
			}
			else {
				_root->GetRange(kMer, iCurrentK, _iMinK, _iMaxLevel, move(start), move(range));
			}
		}
	}
	
	///////////////////////////
	inline bool IsInTrie(const uint64_t kMer) const {
		return _root->isInTrie(static_cast<uint32_t>(kMer >> 30), _ikForIsInTrie, _iMaxLevel);
	}

	///////////////////////////
	inline void TraverseTrieInOrder(const vector<uint64_t>& vIn, vector<uint64_t>& vOut) {
		uint64_t iIdxForOut = 0;
		_root->InOrderTraversal(7, 6, vIn, vOut, iIdxForOut);
	}
};
