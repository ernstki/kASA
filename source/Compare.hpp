/***************************************************************************
*  Part of kASA: https://github.com/SilvioWeging/kASA
*
*  Copyright (C) 2019 Silvio Weging <silvio.weging@gmail.com>
*
*  Distributed under the Boost Software License, Version 1.0.
*  (See accompanying file LICENSE_1_0.txt or copy at
*  http://www.boost.org/LICENSE_1_0.txt)
**************************************************************************/
#pragma once
#include "Read.hpp"
#include "Trie.hpp"

namespace kASA {
	class Compare : public Read {

		const bool _bTranslated = false;

	public:
		Compare(const string& tmpPath, const int32_t& iNumOfProcs, const int32_t& iHigherK, const int32_t& iLowerK, const int32_t& iNumOfCall, const int32_t& iNumOfBeasts, const bool& bVerbose = false, const bool& bProtein = false) : Read(tmpPath, iNumOfProcs, iHigherK, iLowerK, iNumOfCall, bVerbose, bProtein), _bTranslated(bProtein), iNumOfBeasts(iNumOfBeasts) {}

		// for output
		bool bHumanReadable = false;
		int32_t iNumOfBeasts = 3;

	private:

		const float arrWeightingFactors[12] = { 1, 121.f / 144.f , 100.f / 144.f , 81.f / 144.f , 64.f / 144.f , 49.f / 144.f , 36.f / 144.f , 25.f / 144.f , 16.f / 144.f , 9.f / 144.f , 4.f / 144.f , 1.f / 144.f };


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		inline void markTaxIDs(uint16_t codedTaxIDs, Utilities::sBitArray& vMemoryOfTaxIDs_k) {
			try {
				/*while (codedTaxIDs != 0ul) {
					const uint8_t& numOfBits = codedTaxIDs & 31;
					codedTaxIDs >>= 5;
					vMemoryOfTaxIDs_k.set(static_cast<uint16_t>(codedTaxIDs & Utilities::_bitMasks[numOfBits]));
					codedTaxIDs >>= numOfBits;
				}*/
				vMemoryOfTaxIDs_k.set(codedTaxIDs);
			}
			catch (...) {
				throw;
			}
		}
		///////////////////////
		inline void compareWithDatabase(const vector<pair<uint64_t, Utilities::rangeContainer>>& vIn, const unique_ptr<unique_ptr<const index_t_p>[]>& vLib, unique_ptr<double[]>& vCount, unique_ptr<uint64_t[]>& vCountUnique, unique_ptr<float[]>& vReadIDtoGenID, const uint32_t& iSpecIDRange, const unordered_map<readIDType, uint64_t>& mReadIDToArrayIdx) {

#pragma omp parallel
			{
				size_t iParallelCounter = 0;
				int32_t iThreadID = omp_get_thread_num();
				int32_t iNumOfThreads = omp_get_num_threads();
				//size_t iRangeForThreads = vIn.size() / iNumOfThreads;
				//int32_t iUndividablePart = vIn.size() % iNumOfThreads;
				//size_t iStart = (iThreadID == 0) ? 0 : iThreadID * iRangeForThreads + !!(iUndividablePart / (iThreadID));
				//size_t iEnd = (iThreadID + 1)*iRangeForThreads + !!(iUndividablePart / (iThreadID + 1));
				

				try {

					vector<Utilities::vector_set<pair<uint64_t, float>>> vMemoryCounter(_iNumOfK, Utilities::vector_set<pair<uint64_t, float>>(mReadIDToArrayIdx.size(), pair<uint64_t, float>()));
					vector<uint64_t> vMemoryOfSeenkMers(_iNumOfK);
					vector<uint32_t> vMemoryCounterOnly(_iNumOfK, 0);
					vector<Utilities::vector_set<uint64_t>> vMemoryOfReadIDs(_iNumOfK, Utilities::vector_set<uint64_t>(10, 1ull));
					vector<Utilities::sBitArray> vMemoryOfTaxIDs(_iNumOfK, Utilities::sBitArray(iSpecIDRange));

					for (auto mapIt = vIn.cbegin(); mapIt != vIn.cend(); ++mapIt, ++iParallelCounter) {
						//if (iParallelCounter >= iStart && iParallelCounter < iEnd) {
						if (static_cast<int32_t>(iParallelCounter%iNumOfThreads) == iThreadID) {


							const uint64_t& ivInSize = mapIt->second.kMers_GT6.size();

							uint64_t iIdxIn = 0;



							for (int32_t iK = 0; iK < _iNumOfK; ++iK) {
								vMemoryCounter[iK].clear(pair<uint64_t, float>());
								vMemoryOfSeenkMers[iK] = 0;
								vMemoryCounterOnly[iK] = 0;
								vMemoryOfReadIDs[iK].clear(0ull);
								vMemoryOfTaxIDs[iK].clear();
							}

							tuple<uint64_t, uint64_t> iSeenInput = make_pair(0, 0);

							const int32_t& ikDifferenceTop = _iHighestK - _iMaxK;


							const auto libBeginIt = vLib[iThreadID]->cbegin();
							auto seenResultIt = libBeginIt;
							bool bInputIterated = false;

							//uint64_t iBitMask = 1152921503533105152ULL;

							while (iIdxIn < ivInSize) {

								const auto& iCurrentkMer = mapIt->second.kMers_GT6[iIdxIn];

								//cout << kMerToAminoacid(iCurrentkMer.first, 12) << endl;

								// Count duplicates too
								if (get<0>(iSeenInput) == get<0>(iCurrentkMer) && bInputIterated) {
									for (int32_t ikLengthCounter = _iNumOfK - 1; ikLengthCounter >= 0; --ikLengthCounter) {
										const int32_t& shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										const auto& iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										if (iCurrentkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
											++vMemoryCounterOnly[ikLengthCounter];
											vMemoryCounter[ikLengthCounter].insert_pair(make_pair(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second, 1.0f));
											vMemoryOfReadIDs[ikLengthCounter].insert(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second);
										}
									}


									++iIdxIn;
									bInputIterated = true;
									continue;
								}
								else {
									iSeenInput = iCurrentkMer;
								}

								int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1);

								////////////////////////////////////////////////////////////////////////
								int32_t shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);

								auto iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;

								// If the ending is ^ it's not going to hit anyway, might as well stop here
								if ((iCurrentkMerShifted & 31) == 30) {
									++iIdxIn;
									bInputIterated = true;
									continue;
								}

								const auto func = [&shift](const uint64_t& val) { return val >> shift; };
								const auto rangeBeginIt = libBeginIt + mapIt->first, rangeEndIt = libBeginIt + static_cast<uint64_t>(mapIt->first) + mapIt->second.range;

								//auto itResultIterator = rangeEndIt + 1;
								if (func(rangeBeginIt->first) == iCurrentkMerShifted) {
									seenResultIt = rangeBeginIt;
								}
								else {
									if (func(rangeEndIt->first) == iCurrentkMerShifted) {
										// we need the first occurence in the database
										uint64_t iTemp = 1;
										while (func((rangeEndIt - iTemp)->first) == iCurrentkMerShifted) {
											++iTemp;
										}
										seenResultIt = rangeEndIt - (iTemp - 1);
									}
									else {
										if (iCurrentkMerShifted < func(rangeBeginIt->first) || iCurrentkMerShifted > func(rangeEndIt->first)) {
											//cout << kMerToAminoacid(iCurrentkMerShifted,12) << " " << kMerToAminoacid(func(rangeBeginIt->first), 12) << " " << kMerToAminoacid(func(rangeEndIt->first), 12) << endl;
											++iIdxIn;
											bInputIterated = true;
											continue;
										}
										else {
											seenResultIt = lower_bound(rangeBeginIt, rangeEndIt + 1, iCurrentkMerShifted, [&shift](const decltype(*libBeginIt)& a, const uint64_t& val) { return (a.first >> shift) < val; });
										}
									}
								}



								bool bBreakOut = false;
								while (seenResultIt != rangeEndIt + 1 && !bBreakOut) {

									const auto& iCurrentLib = make_tuple(seenResultIt->first, seenResultIt->second);

									for (; ikLengthCounter >= 0; --ikLengthCounter) {

										shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										const auto& iCurrentLibkMerShifted = get<0>(iCurrentLib) >> shift;

										if (iCurrentkMerShifted < iCurrentLibkMerShifted) {
											bBreakOut = true;
											break;
										}
										else {
											if (!(iCurrentLibkMerShifted < iCurrentkMerShifted)) {

												if (iCurrentLibkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
													++vMemoryCounterOnly[ikLengthCounter];
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter]);
													vMemoryOfReadIDs[ikLengthCounter].insert(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second);
													vMemoryCounter[ikLengthCounter].insert_pair(make_pair(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second, 1.0f));
												}
												else {

													const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
													auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
													it.SetNumOfEntries(numOfEntries);
													for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
														const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
														vCount[tempIndex] += double(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;

														if (numOfEntries == 1) {
#pragma omp atomic
															vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
														}

														const auto& entry = *it;
														const auto& weight = arrWeightingFactors[ikDifferenceTop + ikLengthCounter];

														for (const auto& idx : vMemoryOfReadIDs[ikLengthCounter]) {
															const auto& score = vMemoryCounter[ikLengthCounter].findScore(idx) / (1 + logf(static_cast<float>(numOfEntries)));
															const auto& arrayIdx = idx * iSpecIDRange + entry;

															//auto& tempMapVal = mReadIDToTaxID[arrayIdx];
															//tempMapVal = make_tuple(static_cast<uint32_t>(entry), static_cast<uint32_t>(idx), get<2>(tempMapVal) + score * weight);
#pragma omp atomic
															vReadIDtoGenID[arrayIdx] += score * weight;
														}
													}


													vMemoryOfReadIDs[ikLengthCounter].clear(0ull);
													vMemoryOfReadIDs[ikLengthCounter].insert(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second);

													vMemoryOfTaxIDs[ikLengthCounter].clear();
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter]);

													vMemoryCounter[ikLengthCounter].clear(pair<uint64_t, float>());
													vMemoryCounter[ikLengthCounter].insert_pair(make_pair(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second, 1.0f));

													vMemoryCounterOnly[ikLengthCounter] = 1;

													vMemoryOfSeenkMers[ikLengthCounter] = iCurrentLibkMerShifted;
												}

											}
											else {
												uint64_t iTempCounter = 1;
												const uint64_t& iCurrentSuffix = seenResultIt->first;
												while (seenResultIt + iTempCounter != rangeEndIt + 1) {
													const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
													int16_t iUntilK = static_cast<int16_t>(_iNumOfK - 1);
													for (; iUntilK > ikLengthCounter; --iUntilK) {
														if ((iCurrentSuffix >> 5 * iUntilK) == (iNextLibSuffix >> 5 * iUntilK)) {
															markTaxIDs((seenResultIt + iTempCounter)->second, vMemoryOfTaxIDs[iUntilK]);
														}
														else {
															break;
														}
													}
													if (iCurrentSuffix == iNextLibSuffix) {
														++iTempCounter;
													}
													else {
														break;
													}
												}
												seenResultIt += iTempCounter;
												bInputIterated = false;
												break;
											}
										}
									}
									// loop through to find other hits in the library
									if (ikLengthCounter == -1) {
										uint64_t iTempCounter = 1;
										const uint64_t& iCurrentLibSuffix = static_cast<uint64_t>(seenResultIt->first);
										while (seenResultIt + iTempCounter != rangeEndIt + 1) {
											const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
											const auto& iNextLibIdx = (seenResultIt + iTempCounter)->second;
											if (iCurrentLibSuffix == iNextLibSuffix) {
												for (int16_t ikLengthCounter_ = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter_ > ikLengthCounter; --ikLengthCounter_) {
													markTaxIDs(iNextLibIdx, vMemoryOfTaxIDs[ikLengthCounter_]); // to identify multiple hits 
												}

												++iTempCounter;
											}
											else {
												bBreakOut = true;
												break;
											}
										}

										seenResultIt += iTempCounter;
									}
								}
								++iIdxIn;
								bInputIterated = true;
							}


							// Don't forget the last saved part
							for (int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter >= 0; --ikLengthCounter) {
								/*for (auto it = vMemoryOfTaxIDs[ikLengthCounter].begin(); it != vMemoryOfTaxIDs[ikLengthCounter].end(); ++it) {
									const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
									vCount[tempIndex] += float(vMemoryCounterOnly[ikLengthCounter]) / vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();

									if (vMemoryOfTaxIDs[ikLengthCounter].numOfEntries() == 1) {
#pragma omp atomic
										vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
									}
								}


								for (const auto& entry : vMemoryOfTaxIDs[ikLengthCounter]) {
									for (const auto& idx : vMemoryOfReadIDs[ikLengthCounter]) {
#pragma omp atomic
										vReadIDtoGenID[idx*iSpecIDRange + entry] += vMemoryCounter[ikLengthCounter].findScore(idx) * arrWeightingFactors[ikDifferenceTop + ikLengthCounter];
									}
								}*/

								const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();

								auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
								it.SetNumOfEntries(numOfEntries);
								for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
									const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
									vCount[tempIndex] += double(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;

									if (numOfEntries == 1) {
#pragma omp atomic
										vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
									}

									const auto& entry = *it;
									const auto& weight = arrWeightingFactors[ikDifferenceTop + ikLengthCounter];

									for (const auto& idx : vMemoryOfReadIDs[ikLengthCounter]) {
										 
										const auto& score = vMemoryCounter[ikLengthCounter].findScore(idx) / (1 + logf(static_cast<float>(numOfEntries)));
										const auto& arrayIdx = idx * iSpecIDRange + entry;
#pragma omp atomic
										vReadIDtoGenID[arrayIdx] += score * weight;
									}
								}
							}
							//}

						}
					}
				} catch (const exception& e) {
					//cout << e.what() << endl;
#pragma omp critical
					{
						cerr << "ERROR: " << e.what() << endl;
					}
				}
			}
		}

		///////////////////////
		inline void createProfile(const vector<pair<uint64_t, Utilities::rangeContainer>>& vIn, const unique_ptr<unique_ptr<const index_t_p>[]>& vLib, unique_ptr<double[]>& vCount, unique_ptr<uint64_t[]>& vCountUnique, const uint32_t& iSpecIDRange) {

#pragma omp parallel 
			{
				size_t iParallelCounter = 0;
				int32_t iThreadID = omp_get_thread_num();
				int32_t iNumOfThreads = omp_get_num_threads();
				try {

					vector<uint64_t> vMemoryOfSeenkMers(_iNumOfK);
					vector<uint32_t> vMemoryCounterOnly(_iNumOfK, 0);
					vector<Utilities::sBitArray> vMemoryOfTaxIDs(_iNumOfK, Utilities::sBitArray(iSpecIDRange));

					for (auto vecIt = vIn.cbegin(); vecIt != vIn.cend(); ++vecIt, ++iParallelCounter) {

						if (static_cast<int32_t>(iParallelCounter%iNumOfThreads) == iThreadID) {
							const uint64_t& ivInSize = vecIt->second.kMers_GT6.size();

							uint64_t iIdxIn = 0;

							for (int32_t iK = 0; iK < _iNumOfK; ++iK) {
								vMemoryOfSeenkMers[iK] = 0;
								vMemoryCounterOnly[iK] = 0;
								vMemoryOfTaxIDs[iK].clear();
							}

							tuple<uint64_t, uint64_t> iSeenInput = make_pair(0, 0);

							const auto libBeginIt = vLib[iThreadID]->cbegin();
							auto seenResultIt = libBeginIt;

							bool bInputIterated = false;
							//uint64_t iIdxLib = 0;
							//uint64_t iBitMask = 1152921503533105152ULL;
							//uint64_t iLastRangeBegin = 0;

							while (iIdxIn < ivInSize) {

								const auto& iCurrentkMer = vecIt->second.kMers_GT6[iIdxIn];

								// Count duplicates too
								if (get<0>(iSeenInput) == get<0>(iCurrentkMer) && bInputIterated) {
									for (int32_t ikLengthCounter = _iNumOfK - 1; ikLengthCounter >= 0; --ikLengthCounter) {
										const int32_t& shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										const auto& iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										if (iCurrentkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
											++vMemoryCounterOnly[ikLengthCounter];
										}
									}


									++iIdxIn;
									bInputIterated = true;
									continue;
								}
								else {
									iSeenInput = iCurrentkMer;
								}

								int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1);

								////////////////////////////////////////////////////////////////////////
								int32_t shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);

								auto iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;

								// If the ending is ^ it's not going to hit anyway, might as well stop here
								if ((iCurrentkMerShifted & 31) == 30) {
									++iIdxIn;
									bInputIterated = true;
									continue;
								}

								const auto func = [&shift](const uint64_t& val) { return val >> shift; };
								const auto rangeBeginIt = libBeginIt + vecIt->first, rangeEndIt = libBeginIt + static_cast<uint64_t>(vecIt->first) + vecIt->second.range;

								if (func(rangeBeginIt->first) == iCurrentkMerShifted) {
									seenResultIt = rangeBeginIt;
								}
								else {
									if (func(rangeBeginIt->first) == iCurrentkMerShifted) {
										// we need the first occurence in the database
										uint64_t iTemp = 1;
										while (func((rangeEndIt - iTemp)->first) == iCurrentkMerShifted) {
											++iTemp;
										}
										seenResultIt = rangeEndIt - (iTemp - 1);
									}
									else {
										if (iCurrentkMerShifted < func(rangeBeginIt->first) || iCurrentkMerShifted > func(rangeEndIt->first)) {
											++iIdxIn;
											bInputIterated = true;
											continue;
										}
										else {
											seenResultIt = lower_bound(rangeBeginIt, rangeEndIt + 1, iCurrentkMerShifted, [&shift](const decltype(*libBeginIt)& a, const uint64_t& val) { return (a.first >> shift) < val; });
										}
									}
								}

								bool bBreakOut = false;
								while (seenResultIt != rangeEndIt + 1 && !bBreakOut) {
									const auto& iCurrentLib = make_tuple(seenResultIt->first, seenResultIt->second);
									//cout << get<0>(iCurrentLib) << " " << get<1>(iCurrentLib) << endl;

									for (; ikLengthCounter >= 0; --ikLengthCounter) {

										shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										const auto& iCurrentLibkMerShifted = get<0>(iCurrentLib) >> shift;

										if (iCurrentkMerShifted < iCurrentLibkMerShifted) {
											bBreakOut = true;
											break;
										}
										else {
											if (!(iCurrentLibkMerShifted < iCurrentkMerShifted)) {

												if (iCurrentLibkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
													++vMemoryCounterOnly[ikLengthCounter];
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter]);
												}
												else {
													const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
													auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
													it.SetNumOfEntries(numOfEntries);
													for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
														const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
														vCount[tempIndex] += double(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;


														if (numOfEntries == 1) {
#pragma omp atomic
															vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
														}
													}

													vMemoryOfTaxIDs[ikLengthCounter].clear();
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter]);

													vMemoryCounterOnly[ikLengthCounter] = 1;

													vMemoryOfSeenkMers[ikLengthCounter] = iCurrentLibkMerShifted;
												}
											}
											else {
												uint64_t iTempCounter = 1;
												const uint64_t& iCurrentLibSuffix = static_cast<uint64_t>(seenResultIt->first);
												while (seenResultIt + iTempCounter != rangeEndIt + 1) {
													const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
													int16_t iUntilK = static_cast<int16_t>(_iNumOfK - 1);
													for (; iUntilK > ikLengthCounter; --iUntilK) {
														if ((iCurrentLibSuffix >> 5 * iUntilK) == (iNextLibSuffix >> 5 * iUntilK)) {
															markTaxIDs((seenResultIt + iTempCounter)->second, vMemoryOfTaxIDs[iUntilK]);
														}
														else {
															break;
														}
													}
													if (iCurrentLibSuffix == iNextLibSuffix) {
														++iTempCounter;
													}
													else {
														break;
													}
												}
												seenResultIt += iTempCounter;
												bInputIterated = false;
												break;
											}
										}
									}
									// loop through to find other hits in the library
									if (ikLengthCounter == -1) {
										uint64_t iTempCounter = 1;
										const uint64_t& iCurrentLibSuffix = static_cast<uint64_t>(seenResultIt->first);
										while (seenResultIt + iTempCounter != rangeEndIt + 1) {
											const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
											const auto& iNextLibIdx = (seenResultIt + iTempCounter)->second;
											if (iCurrentLibSuffix == iNextLibSuffix) {
												for (int16_t ikLengthCounter_ = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter_ > ikLengthCounter; --ikLengthCounter_) {
													markTaxIDs(iNextLibIdx, vMemoryOfTaxIDs[ikLengthCounter_]); // to identify multiple hits  
												}

												++iTempCounter;
											}
											else {
												bBreakOut = true;
												break;
											}
										}

										seenResultIt += iTempCounter;
									}
								}
								++iIdxIn;
								bInputIterated = true;
							}


							// Don't forget the last saved part
							for (int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter >= 0; --ikLengthCounter) {
								const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
								auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
								it.SetNumOfEntries(numOfEntries);
								for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
									const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
									vCount[tempIndex] += double(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;


									if (numOfEntries == 1) {
#pragma omp atomic
										vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
									}
								}
							}
						}
					}
				}
				catch (const exception& e) {
#pragma omp critical
					{
						cerr << "ERROR: " << e.what() << endl;
					}
				}
			}
		}


		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline void markTaxIDs(const uint64_t& codedTaxIDs, Utilities::sBitArray& vMemoryOfTaxIDs_k, const unordered_map<uint32_t, uint32_t>& mTaxToIdx, unique_ptr<const contentVecType_32p>&) {
			try {
				vMemoryOfTaxIDs_k.set(Utilities::checkIfInMap(mTaxToIdx, static_cast<uint32_t>(codedTaxIDs))->second);
			} 
			catch (...) {
				throw;
			}
		}
		inline void markTaxIDs(const uint64_t& codedTaxIDs, Utilities::sBitArray& vMemoryOfTaxIDs_k, const unordered_map<uint32_t, uint32_t>& mTaxToIdx, const vector<tuple<uint64_t, uint32_t>>*) {
			try {
				vMemoryOfTaxIDs_k.set(Utilities::checkIfInMap(mTaxToIdx, static_cast<uint32_t>(codedTaxIDs))->second);
			}
			catch (...) {
				throw;
			}
		}
		inline void markTaxIDs(const uint64_t& codedTaxIDs, Utilities::sBitArray& vMemoryOfTaxIDs_k, const unordered_map<uint32_t, uint32_t>& mTaxToIdx, const vector<tuple<uint32_t, uint32_t>>*) {
			try {
				vMemoryOfTaxIDs_k.set(Utilities::checkIfInMap(mTaxToIdx, static_cast<uint32_t>(codedTaxIDs))->second);
			}
			catch (...) {
				throw;
			}
		}


		///////////////////////////////////////////////////////
		inline unique_ptr<const contentVecType_32p>& getVec(const unique_ptr<unique_ptr<const contentVecType_32p>[]>& vec, const int& iThreadID) {
			return vec[iThreadID];
		}

		inline const unique_ptr<const contentVecType_32p[]>& getVec(const unique_ptr<const contentVecType_32p[]>& vec, const int&) {
			return vec;
		}

		inline const vector<tuple<uint64_t, uint32_t>>* getVec(const vector<tuple<uint64_t, uint32_t>>* vec, const int&) {
			return vec;
		}

		inline const vector<tuple<uint32_t, uint32_t>>* getVec(const vector<tuple<uint32_t, uint32_t>>* vec, const int&) {
			return vec;
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Compare as many as #Number-of-processors vectors with an index lying on a HDD/SSD and note all similarities for any k. 
		// To minimize hard disk access, the order is as follows: Get kMer from RAM Vec -> Search in Prefix-Trie -> Get range of possible hit -> binary search in that range -> note if hit
		template <typename vecType>
		inline void compareWithDatabase(const vector<pair<uint64_t, Utilities::rangeContainer>>& vIn, const vecType& vLib, unique_ptr<double[]>& vCount, unique_ptr<uint64_t[]>& vCountUnique, unique_ptr<float[]>& vReadIDtoGenID, const uint32_t& iSpecIDRange, const unordered_map<uint32_t, uint32_t>& mTaxToIdx, const unordered_map<readIDType, uint64_t>& mReadIDToArrayIdx) {

#pragma omp parallel
			{
				size_t iParallelCounter = 0;
				int32_t iThreadID = omp_get_thread_num();
				int32_t iNumOfThreads = omp_get_num_threads();
				try {

					vector<Utilities::vector_set<pair<uint64_t, float>>> vMemoryCounter(_iNumOfK, Utilities::vector_set<pair<uint64_t, float>>(mReadIDToArrayIdx.size(), pair<uint64_t, float>()));
					vector<uint64_t> vMemoryOfSeenkMers(_iNumOfK);
					vector<uint32_t> vMemoryCounterOnly(_iNumOfK, 0);
					vector<Utilities::vector_set<uint64_t>> vMemoryOfReadIDs(_iNumOfK, Utilities::vector_set<uint64_t>(10, 1ull));
					vector<Utilities::sBitArray> vMemoryOfTaxIDs(_iNumOfK, Utilities::sBitArray(iSpecIDRange));

					for (auto mapIt = vIn.cbegin(); mapIt != vIn.cend(); ++mapIt, ++iParallelCounter) {
						if (static_cast<int32_t>(iParallelCounter%iNumOfThreads) == iThreadID) {

							const uint64_t& ivInSize = (_iMinK <= 6) ? mapIt->second.kMers_ST6.size() : mapIt->second.kMers_GT6.size();

							uint64_t iIdxIn = 0;

							for (int32_t iK = 0; iK < _iNumOfK; ++iK) {
								vMemoryCounter[iK].clear(pair<uint64_t, float>());
								vMemoryOfSeenkMers[iK] = 0;
								vMemoryCounterOnly[iK] = 0;
								vMemoryOfReadIDs[iK].clear(0ull);
								vMemoryOfTaxIDs[iK].clear();
							}

							tuple<uint64_t, uint64_t> iSeenInput = make_pair(0, 0);

							const int32_t& ikDifferenceTop = _iHighestK - _iMaxK;

							const auto libBeginIt = getVec(vLib, iThreadID)->cbegin();
							auto seenResultIt = libBeginIt;
							bool bInputIterated = false;
							//uint64_t iIdxLib = 0;
							//uint64_t iBitMask = 1152921503533105152ULL;
							//uint64_t iLastRangeBegin = 0;

							while (iIdxIn < ivInSize) {

								const pair<uint64_t,uint64_t>& iCurrentkMer = (_iMinK <= 6) ? static_cast<pair<uint64_t, uint64_t>>(mapIt->second.kMers_ST6[iIdxIn]) : static_cast<pair<uint64_t, uint64_t>>(mapIt->second.kMers_GT6[iIdxIn]);
								//cout << kMerToAminoacid(get<0>(iCurrentkMer), 12) << endl;

								// Count duplicates too
								if (get<0>(iSeenInput) == get<0>(iCurrentkMer) && bInputIterated) {
									for (int32_t ikLengthCounter = _iNumOfK - 1; ikLengthCounter >= 0; --ikLengthCounter) {
										const int32_t& shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										const auto& iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										if (iCurrentkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
											++vMemoryCounterOnly[ikLengthCounter];
											vMemoryCounter[ikLengthCounter].insert_pair(make_pair(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second, 1.0f));
											vMemoryOfReadIDs[ikLengthCounter].insert(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second);
										}
									}


									++iIdxIn;
									bInputIterated = true;
									continue;
								}
								else {
									iSeenInput = iCurrentkMer;
								}

								int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1);

								////////////////////////////////////////////////////////////////////////
								int32_t shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);

								auto iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
								//cout << kMerToAminoacid(iPrefix, 12) << endl;

								// If the ending is ^ it's not going to hit anyway, might as well stop here
								if ((iCurrentkMerShifted & 31) == 30) {
									++iIdxIn;
									bInputIterated = true;
									continue;
								}

								
								const auto shiftVal = [&shift,this](const uint64_t& val) { return (_iMinK > 6) ? (val & 1073741823ULL) >> shift : (val >> shift); };
								const auto rangeBeginIt = libBeginIt + mapIt->first, rangeEndIt = libBeginIt + static_cast<uint64_t>(mapIt->first) + mapIt->second.range;


								if (shiftVal(rangeBeginIt->first) == iCurrentkMerShifted) {
									seenResultIt = rangeBeginIt;
								}
								else {
									if (shiftVal(rangeEndIt->first) == iCurrentkMerShifted) {
										// we need the first occurence in the database
										uint64_t iTemp = 1;
										while (shiftVal((rangeEndIt - iTemp)->first) == iCurrentkMerShifted) {
											++iTemp;
										}
										seenResultIt = rangeEndIt - (iTemp - 1);
									}
									else {
										if (iCurrentkMerShifted < shiftVal(rangeBeginIt->first) || iCurrentkMerShifted > shiftVal(rangeEndIt->first)) {
											//cout << kMerToAminoacid(get<0>(iCurrentkMer),12) << " " << kMerToAminoacid(iCurrentkMerShifted,12) << " " << kMerToAminoacid(rangeBeginIt->first, 12) << " " << kMerToAminoacid(shiftVal(rangeBeginIt->first), 12) << " " << kMerToAminoacid(shiftVal(rangeEndIt->first), 12) << endl;
											++iIdxIn;
											bInputIterated = true;
											continue;
										}
										else {
											seenResultIt = lower_bound(rangeBeginIt, rangeEndIt + 1, iCurrentkMerShifted, [&shift, this](const decltype(*libBeginIt)& a, const uint64_t& val) { return (_iMinK > 6) ? ((a.first & 1073741823ULL) >> shift) < val : (a.first >> shift) < val; });
										}
									}
								}

								bool bBreakOut = false;
								while (seenResultIt != rangeEndIt + 1 && !bBreakOut) {

									const tuple<uint64_t,uint32_t>& iCurrentLib = (_iMinK > 6) ? static_cast<tuple<uint64_t, uint32_t>>(make_tuple(seenResultIt->first & 1073741823ULL, seenResultIt->second)) : tuple<uint64_t, uint32_t>(make_tuple(seenResultIt->first, seenResultIt->second));

									for (; ikLengthCounter >= 0; --ikLengthCounter) {

										shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										const auto& iCurrentLibkMerShifted = get<0>(iCurrentLib) >> shift;

										if (iCurrentkMerShifted < iCurrentLibkMerShifted) {
											bBreakOut = true;
											break;
										}
										else {
											if (!(iCurrentLibkMerShifted < iCurrentkMerShifted)) {

												if (iCurrentLibkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {

													++vMemoryCounterOnly[ikLengthCounter];

													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter], mTaxToIdx, getVec(vLib, iThreadID));
													vMemoryOfReadIDs[ikLengthCounter].insert(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second);
													vMemoryCounter[ikLengthCounter].insert_pair(make_pair(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second, 1.0f));
												}
												else {
													const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
													auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
													it.SetNumOfEntries(numOfEntries);
													for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
														const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
														vCount[tempIndex] += double(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;

														if (numOfEntries == 1) {
#pragma omp atomic
															vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
														}

														const auto& entry = *it;
														const auto& weight = arrWeightingFactors[ikDifferenceTop + ikLengthCounter];

														for (const auto& idx : vMemoryOfReadIDs[ikLengthCounter]) {
															const auto& score = vMemoryCounter[ikLengthCounter].findScore(idx) / (1 + logf(static_cast<float>(numOfEntries)));
															const auto& arrayIdx = idx * iSpecIDRange + entry;
#pragma omp atomic
															vReadIDtoGenID[arrayIdx] += score * weight;
														}
													}



													vMemoryOfReadIDs[ikLengthCounter].clear(0ull);
													vMemoryOfReadIDs[ikLengthCounter].insert(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second);

													vMemoryOfTaxIDs[ikLengthCounter].clear();
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter], mTaxToIdx, getVec(vLib, iThreadID));

													vMemoryCounter[ikLengthCounter].clear(pair<uint64_t, float>());
													vMemoryCounter[ikLengthCounter].insert_pair(make_pair(Utilities::checkIfInMap(mReadIDToArrayIdx, get<1>(iCurrentkMer))->second, 1.0f));

													vMemoryCounterOnly[ikLengthCounter] = 1;

													vMemoryOfSeenkMers[ikLengthCounter] = iCurrentLibkMerShifted;
												}

											}
											else {
												uint64_t iTempCounter = 1;
												const uint64_t& iCurrentLibSuffix = seenResultIt->first;
												while (seenResultIt + iTempCounter != rangeEndIt + 1) {
													const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
													int16_t iUntilK = static_cast<int16_t>(_iNumOfK - 1);
													for (; iUntilK > ikLengthCounter; --iUntilK) {
														if ((iCurrentLibSuffix >> 5 * iUntilK) == (iNextLibSuffix >> 5 * iUntilK)) {
															markTaxIDs((seenResultIt + iTempCounter)->second, vMemoryOfTaxIDs[iUntilK], mTaxToIdx, getVec(vLib, iThreadID));
														}
														else {
															break;
														}
													}
													if (iCurrentLibSuffix == iNextLibSuffix) {
														++iTempCounter;
													}
													else {
														break;
													}
												}
												//iIdxLib += iTempCounter;
												seenResultIt += iTempCounter;
												bInputIterated = false;
												break;
											}
										}
									}
									// loop through to find other hits in the library
									if (ikLengthCounter == -1) {
										uint64_t iTempCounter = 1;
										const uint64_t& iCurrentLibSuffix = static_cast<uint64_t>(seenResultIt->first);
										while (seenResultIt + iTempCounter != rangeEndIt + 1) {
											const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
											const auto& iNextLibIdx = (seenResultIt + iTempCounter)->second;
											if (iCurrentLibSuffix == iNextLibSuffix) {
												for (int16_t ikLengthCounter_ = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter_ > ikLengthCounter; --ikLengthCounter_) {
													markTaxIDs(iNextLibIdx, vMemoryOfTaxIDs[ikLengthCounter_], mTaxToIdx, getVec(vLib, iThreadID)); // to identify multiple hits 
												}

												++iTempCounter;
											}
											else {
												bBreakOut = true;
												break;
											}
										}

										seenResultIt += iTempCounter;
									}
								}
								++iIdxIn;
								bInputIterated = true;
							}


							// Don't forget the last saved part
							for (int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter >= 0; --ikLengthCounter) {
								const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
								auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
								it.SetNumOfEntries(numOfEntries);
								for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
									const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
									vCount[tempIndex] += float(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;

									if (numOfEntries == 1) {
#pragma omp atomic
										vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
									}

									const auto& entry = *it;
									const auto& weight = arrWeightingFactors[ikDifferenceTop + ikLengthCounter];

									for (const auto& idx : vMemoryOfReadIDs[ikLengthCounter]) {
										const auto& score = vMemoryCounter[ikLengthCounter].findScore(idx) / (1 + logf(static_cast<float>(numOfEntries)));
										const auto& arrayIdx = idx * iSpecIDRange + entry;
#pragma omp atomic
										vReadIDtoGenID[arrayIdx] += score * weight;
									}
								}
							}
						}
					}
				}
				catch (const exception& e) {
#pragma omp critical
					{
						cerr << "ERROR: " << e.what() << endl;
					}
				}
			}
		}


		template <typename vecType>
		inline void createProfile(const vector<pair<uint64_t, Utilities::rangeContainer>>& vIn, const vecType& vLib, unique_ptr<double[]>& vCount, unique_ptr<uint64_t[]>& vCountUnique, const uint32_t& iSpecIDRange, const unordered_map<uint32_t, uint32_t>& mTaxToIdx) {

#pragma omp parallel 
			{
				size_t iParallelCounter = 0;
				int32_t iThreadID = omp_get_thread_num();
				int32_t iNumOfThreads = omp_get_num_threads();
				try {

					vector<uint64_t> vMemoryOfSeenkMers(_iNumOfK);
					vector<uint32_t> vMemoryCounterOnly(_iNumOfK, 0);
					vector<Utilities::sBitArray> vMemoryOfTaxIDs(_iNumOfK, Utilities::sBitArray(iSpecIDRange));

					for (auto vecIt = vIn.cbegin(); vecIt != vIn.cend(); ++vecIt, ++iParallelCounter) {
						if (static_cast<int32_t>(iParallelCounter%iNumOfThreads) == iThreadID) {

							const uint64_t& ivInSize = (_iMinK <= 6) ? vecIt->second.kMers_ST6.size() : vecIt->second.kMers_GT6.size();

							uint64_t iIdxIn = 0;

							for (int32_t iK = 0; iK < _iNumOfK; ++iK) {
								vMemoryOfSeenkMers[iK] = 0;
								vMemoryCounterOnly[iK] = 0;
								vMemoryOfTaxIDs[iK].clear();
							}

							tuple<uint64_t, uint64_t> iSeenInput = make_pair(0, 0);

							const auto libBeginIt = getVec(vLib, iThreadID)->cbegin();
							auto seenResultIt = libBeginIt;

							bool bInputIterated = false;
							//uint64_t iIdxLib = 0;
							//uint64_t iLastRangeBegin = 0;

							while (iIdxIn < ivInSize) {

								const pair<uint64_t,uint64_t>& iCurrentkMer = (_iMinK <= 6) ? static_cast<pair<uint64_t, uint64_t>>(vecIt->second.kMers_ST6[iIdxIn]) : static_cast<pair<uint64_t, uint64_t>>(vecIt->second.kMers_GT6[iIdxIn]);
								

								// Count duplicates too
								if (get<0>(iSeenInput) == get<0>(iCurrentkMer) && bInputIterated) {
									for (int32_t ikLengthCounter = _iNumOfK - 1; ikLengthCounter >= 0; --ikLengthCounter) {
										const int32_t& shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										const auto& iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										if (iCurrentkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
											++vMemoryCounterOnly[ikLengthCounter];
										}
									}


									++iIdxIn;
									bInputIterated = true;
									continue;
								}
								else {
									iSeenInput = iCurrentkMer;
								}

								int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1);

								////////////////////////////////////////////////////////////////////////
								int32_t shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);

								auto iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;

								// If the ending is ^ it's not going to hit anyway, might as well stop here
								if ((iCurrentkMerShifted & 31) == 30) {
									++iIdxIn;
									bInputIterated = true;
									continue;
								}

								const auto shiftVal = [&shift, this](const uint64_t& val) { return (_iMinK > 6) ? (val & 1073741823ULL) >> shift : (val >> shift); };
								const auto rangeBeginIt = libBeginIt + vecIt->first, rangeEndIt = libBeginIt + static_cast<uint64_t>(vecIt->first) + vecIt->second.range;

								if (shiftVal(rangeBeginIt->first) == iCurrentkMerShifted) {
									seenResultIt = rangeBeginIt;
								}
								else {
									if (shiftVal(rangeBeginIt->first) == iCurrentkMerShifted) {
										// we need the first occurence in the database
										uint64_t iTemp = 1;
										while (shiftVal((rangeEndIt - iTemp)->first) == iCurrentkMerShifted) {
											++iTemp;
										}
										seenResultIt = rangeEndIt - (iTemp - 1);
									}
									else {
										if (iCurrentkMerShifted < shiftVal(rangeBeginIt->first) || iCurrentkMerShifted > shiftVal(rangeEndIt->first)) {
											++iIdxIn;
											bInputIterated = true;
											continue;
										}
										else {
											seenResultIt = lower_bound(rangeBeginIt, rangeEndIt + 1, iCurrentkMerShifted, [&shift,this](const decltype(*libBeginIt)& a, const uint64_t& val) { return (_iMinK > 6) ? ((a.first & 1073741823ULL) >> shift) < val : (a.first >> shift) < val; });
										}
									}
								}

								bool bBreakOut = false;
								while (seenResultIt != rangeEndIt + 1 && !bBreakOut) {
									const tuple<uint64_t, uint32_t>& iCurrentLib = (_iMinK > 6) ? static_cast<tuple<uint64_t, uint32_t>>(make_tuple(seenResultIt->first & 1073741823ULL, seenResultIt->second)) : tuple<uint64_t, uint32_t>(make_tuple(seenResultIt->first, seenResultIt->second));
									//cout << get<0>(iCurrentLib) << " " << get<1>(iCurrentLib) << endl;

									for (; ikLengthCounter >= 0; --ikLengthCounter) {

										shift = 5 * (_iHighestK - _aOfK[ikLengthCounter]);
										iCurrentkMerShifted = get<0>(iCurrentkMer) >> shift;
										const auto& iCurrentLibkMerShifted = get<0>(iCurrentLib) >> shift;

										if (iCurrentkMerShifted < iCurrentLibkMerShifted) {
											bBreakOut = true;
											break;
										}
										else {
											if (!(iCurrentLibkMerShifted < iCurrentkMerShifted)) {
												/*if (!bInputIterated) {
													if (iCurrentLibkMerShifted == vMemoryOfSeenkMers[ikLengthCounter] && get<1>(iCurrentLib) == vMemoryOfSeenTaxIDs[ikLengthCounter]) {
														continue;
													}
													else {
														vMemoryOfSeenTaxIDs[ikLengthCounter] = get<1>(iCurrentLib);
													}
												}*/

												if (iCurrentLibkMerShifted == vMemoryOfSeenkMers[ikLengthCounter]) {
													++vMemoryCounterOnly[ikLengthCounter];
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter], mTaxToIdx, getVec(vLib, iThreadID));
												}
												else {
													const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
													auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
													it.SetNumOfEntries(numOfEntries);
													for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
														const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
														vCount[tempIndex] += double(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;


														if (numOfEntries == 1) {
#pragma omp atomic
															vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
														}
													}

													vMemoryOfTaxIDs[ikLengthCounter].clear();
													markTaxIDs(get<1>(iCurrentLib), vMemoryOfTaxIDs[ikLengthCounter], mTaxToIdx, getVec(vLib, iThreadID));

													vMemoryCounterOnly[ikLengthCounter] = 1;

													vMemoryOfSeenkMers[ikLengthCounter] = iCurrentLibkMerShifted;
												}
											}
											else {
												uint64_t iTempCounter = 1;
												const uint64_t& iCurrentLibSuffix = static_cast<uint64_t>(seenResultIt->first);
												while (seenResultIt + iTempCounter != rangeEndIt + 1) {
													const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
													int16_t iUntilK = static_cast<int16_t>(_iNumOfK - 1);
													for (; iUntilK > ikLengthCounter; --iUntilK) {
														if ((iCurrentLibSuffix >> 5 * iUntilK) == (iNextLibSuffix >> 5 * iUntilK)) {
															markTaxIDs((seenResultIt + iTempCounter)->second, vMemoryOfTaxIDs[iUntilK], mTaxToIdx, getVec(vLib, iThreadID));
														}
														else {
															break;
														}
													}
													if (iCurrentLibSuffix == iNextLibSuffix) {
														++iTempCounter;
													}
													else {
														break;
													}
												}
												//iIdxLib += iTempCounter;
												seenResultIt += iTempCounter;
												bInputIterated = false;
												break;
											}
										}
									}
									// loop through to find other hits in the library
									if (ikLengthCounter == -1) {
										uint64_t iTempCounter = 1;
										const uint64_t& iCurrentLibSuffix = static_cast<uint64_t>(seenResultIt->first);
										while (seenResultIt + iTempCounter != rangeEndIt + 1) {
											const uint64_t& iNextLibSuffix = static_cast<uint64_t>((seenResultIt + iTempCounter)->first);
											const auto& iNextLibIdx = (seenResultIt + iTempCounter)->second;
											if (iCurrentLibSuffix == iNextLibSuffix) {
												for (int16_t ikLengthCounter_ = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter_ > ikLengthCounter; --ikLengthCounter_) {
													markTaxIDs(iNextLibIdx, vMemoryOfTaxIDs[ikLengthCounter_], mTaxToIdx, getVec(vLib, iThreadID)); // to identify multiple hits  
												}

												++iTempCounter;
											}
											else {
												bBreakOut = true;
												break;
											}
										}

										seenResultIt += iTempCounter;
									}
								}	
								++iIdxIn;
								bInputIterated = true;
							}


							// Don't forget the last saved part
							for (int16_t ikLengthCounter = static_cast<int16_t>(_iNumOfK - 1); ikLengthCounter >= 0; --ikLengthCounter) {
								const auto& numOfEntries = vMemoryOfTaxIDs[ikLengthCounter].numOfEntries();
								auto it = vMemoryOfTaxIDs[ikLengthCounter].begin();
								it.SetNumOfEntries(numOfEntries);
								for (; it != vMemoryOfTaxIDs[ikLengthCounter].end() && numOfEntries != 0; ++it) {
									const auto& tempIndex = (*it)*_iNumOfK + ikLengthCounter;
#pragma omp atomic
									vCount[tempIndex] += float(vMemoryCounterOnly[ikLengthCounter]) / numOfEntries;

									if (numOfEntries == 1) {
#pragma omp atomic
										vCountUnique[tempIndex] += vMemoryCounterOnly[ikLengthCounter];
									}
								}
							}
						}
					}
				}
				catch (const exception& e) {
#pragma omp critical
					{
						cerr << "ERROR: " << e.what() << endl;
					}
				}
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		inline void scoringFunc(vector<tuple<readIDType, float, double>>&& vTempResultVec, const uint64_t& iReadNum, const pair<string, uint32_t>& vReadNameAndLength, const unordered_map<string, vector<uint64_t>>& mFrequencies, const unordered_map<uint64_t, string>& mOrganisms, ofstream&& fOut) {
			try {
				for (auto it = vTempResultVec.begin(); it != vTempResultVec.end();  ++it) {
					if (_bTranslated) {
						get<2>(*it) = double(get<1>(*it)) / (1.0 + log2((Utilities::checkIfInMap(mFrequencies, Utilities::checkIfInMap(mOrganisms, get<0>(*it))->second)->second)[0] * double(vReadNameAndLength.second - _iHighestK + 1)));
					}
					else {
						get<2>(*it) = double(get<1>(*it)) / (1.0 + log2((Utilities::checkIfInMap(mFrequencies, Utilities::checkIfInMap(mOrganisms, get<0>(*it))->second)->second)[0] * double(vReadNameAndLength.second - _iHighestK * 3 + 1)));
					}
				}

				sort(vTempResultVec.begin(), vTempResultVec.end(), [](const tuple<uint64_t, float, double>& p1, const tuple<uint64_t, float, double>& p2) { return get<2>(p1) > get<2>(p2); });

				if (bHumanReadable) {
					string sOut = "", sOut2 = "", sOut3 = "";

					sOut += to_string(iReadNum) + "\t" + vReadNameAndLength.first + "\t";
					auto it = vTempResultVec.begin();
					float iValueBefore = 0;
					for (int16_t j = 0; it != vTempResultVec.end() && j < iNumOfBeasts; ++it) {
						sOut += to_string(get<0>(*it)) + ";";
						ostringstream e_value;
						e_value.precision(5);
						e_value << std::scientific << get<2>(*it) << "," << std::defaultfloat << get<1>(*it);

						sOut2 += Utilities::checkIfInMap(mOrganisms, get<0>(*it))->second + ";";
						sOut3 += e_value.str() + ";";

						if (iValueBefore != get<1>(*it)) {
							iValueBefore = get<1>(*it);
							++j;
						}
					}
					if (sOut.back() == ';') {
						sOut.pop_back();
					}
					if (sOut2.back() == ';') {
						sOut2.pop_back();
					}
					if (sOut3.back() == ';') {
						sOut3.pop_back();
					}
					if (sOut2.length()) {
						fOut << sOut + "\t" + sOut2 + "\t" + sOut3 << endl;
					}
				}
				else {
					// json
					if (iReadNum == 0) {
						fOut << "{" << endl;
					}
					else {
						fOut << "," << endl << "{" << endl;
					}

					fOut << "\t\"Read number\": " << iReadNum << "," << endl << "\t\"Specifier from input file\": \"" + vReadNameAndLength.first + "\"," << endl << "\t\"Matched taxa\": [" << endl;
					auto it = vTempResultVec.begin();
					float iValueBefore = 0;
					for (int16_t j = 0; it != vTempResultVec.end() && j < iNumOfBeasts; ++it) {
						if (j == 0) {
							fOut << "\t{" << endl;
						}
						else {
							fOut << "," << endl << "\t{" << endl;
						}
						fOut << "\t\t\"tax ID\": \"" << get<0>(*it) << "\"," << endl;
						fOut << "\t\t\"Name\": \"" << Utilities::checkIfInMap(mOrganisms, get<0>(*it))->second << "\"," << endl;
						fOut << "\t\t\"k-mer Score\": " << std::defaultfloat << get<1>(*it) << "," << endl;
						fOut << "\t\t\"Relative Score\": " << std::scientific << get<2>(*it) << endl;
						fOut << "\t}";
						if (iValueBefore != get<1>(*it)) {
							iValueBefore = get<1>(*it);
							++j;
						}
					}

					fOut << endl << "\t]" << endl << "}";
				}
			}
			catch (...) {
				throw;
			}
		}

	public:
		/////////////////////////////////////////////////////////////////////////////////
		void CompareWithLib_partialSort(const string& contentFile, const string& sLibFile, const string& fInFile, const string& fOutFile, const string& fTableFile, const uint8_t& iTrieDepth, const uint64_t& iMemory, const bool& bSpaced) {
			try {
				// test if files exists
				if (!ifstream(contentFile) || !ifstream(sLibFile) || !ifstream(sLibFile + "_f.txt") || !ifstream(sLibFile + "_trie.txt")) {
					throw runtime_error("One of the files does not exist");
				}
				// get names of idxes
				ifstream fContent(contentFile);
				uint32_t iAmountOfSpecies = 1;
				string sTempLine = "";
				unordered_map<uint64_t, string> mOrganisms;
				unordered_map<uint32_t, uint32_t> mTaxToIdx, mIdxToTax;
				mOrganisms[0] = "non_unique";
				mTaxToIdx[0] = 0;
				mIdxToTax[0] = 0;
				while (getline(fContent, sTempLine)) {
					if (sTempLine != "") {
						const auto& tempLineContent = Utilities::split(sTempLine, '\t');
						mOrganisms[stoul(tempLineContent[1])] = tempLineContent[0];
						mIdxToTax[iAmountOfSpecies] = stoul(tempLineContent[1]);
						mTaxToIdx[stoul(tempLineContent[1])] = iAmountOfSpecies++;
					}
				}
				fContent.close();

				// get frequencies
				ifstream fFrequencies(sLibFile + "_f.txt");
				unordered_map<string, vector<uint64_t>> mFrequencies;
				while (getline(fFrequencies, sTempLine)) {
					if (sTempLine != "") {
						const auto& vLine = Utilities::split(sTempLine, '\t');
						vector<uint64_t> vFreqs(_iHighestK - _iLowestK + 1);
						for (int32_t i = 0; i < _iHighestK - _iLowestK + 1; ++i) {
							vFreqs[i] = stoull(vLine[i + 1]);
						}
						mFrequencies[vLine[0]] = vFreqs;
					}
				}


				// get Size of Lib and open it for reading
				ifstream fLibInfo(sLibFile + "_info.txt");
				uint64_t iSizeOfLib = 0;
				fLibInfo >> iSizeOfLib;
				bool bPartitioned = false;
				uint32_t iTempVecType = 0;
				fLibInfo >> iTempVecType;

				if (iTempVecType == 3) {
					bPartitioned = true;
				}
				fLibInfo.close();

				// Assert, that if the index was shrunken via trieHalf MinK can only be larger than 6
				if (bPartitioned && _iMinK <= 6) {
					cerr << "ERROR: k can only be larger than 6 if your index was shrunken via strategy 2. Setting it to 7..." << endl;
					_iMinK = 7;
					_iNumOfK = _iMaxK - _iMinK + 1;
				}

				//unique_ptr<unique_ptr<stxxlFile>[]> stxxlLibFile;
				unique_ptr<stxxlFile> stxxlLibFile(new stxxlFile(sLibFile, stxxl::file::RDONLY));
				unique_ptr<unique_ptr<const contentVecType_32p>[]> vLib;
				unique_ptr<unique_ptr<const index_t_p>[]> vLibParted_p;

				//vector<tuple<uint64_t, uint32_t>> vLibInRAM;

				uint64_t iBytesUsedByVectors = 0;
				if (bPartitioned) {
					vLibParted_p.reset(new unique_ptr<const index_t_p>[_iNumOfThreads]);
					//stxxlLibFile.reset(new unique_ptr<stxxlFile>[_iNumOfThreads]);
					for (int32_t i = 0; i < _iNumOfThreads; ++i) {
						//stxxlLibFile[i].reset(new stxxlFile(sLibFile + "_"+ to_string(i), stxxl::file::RDONLY));
						vLibParted_p[i].reset(new const index_t_p(stxxlLibFile.get(), iSizeOfLib));
					}
					iBytesUsedByVectors = _iNumOfThreads * index_t_p::block_size * index_t_p::page_size * (vLibParted_p[0])->numpages(); // TODO maybe that's not right
				}
				else {
					vLib.reset(new unique_ptr<const contentVecType_32p>[_iNumOfThreads]);
					for (int32_t i = 0; i < _iNumOfThreads; ++i) {
						vLib[i].reset(new const contentVecType_32p(stxxlLibFile.get(), iSizeOfLib));
					}
					iBytesUsedByVectors = _iNumOfThreads * contentVecType_32p::block_size * contentVecType_32p::page_size * (vLib[0])->numpages(); // TODO maybe that's not right
				}
				

				// load Trie
				const uint8_t& iTD = iTrieDepth;
				Trie T(static_cast<int8_t>(_iMaxK), static_cast<int8_t>(_iMinK), iTD, _iNumOfThreads);
				T.LoadFromStxxlVec(sLibFile);
				T.SetForIsInTrie( (_iMinK < 6) ? static_cast<uint8_t>(_iMinK) : static_cast<uint8_t>(6));

				// This holds the hits for each organism
				const uint64_t& iMult = _iNumOfK * uint64_t(iAmountOfSpecies);
				unique_ptr<double[]> vCount_all(new double[iMult]);
				unique_ptr<uint64_t[]> vCount_unique(new uint64_t[iMult]);

				for (uint64_t i = 0; i < iMult; ++i) {
					vCount_all[i] = 0.;
					vCount_unique[i] = 0;
				}

				uint64_t iTimeFastq = 0, iTimeCompare = 0, iNumOfReads = 0, iNumOfReadsOld = 0, iNumOfReadsSum = 0, iSoftMaxSizeOfInputVecs = 0;
				
				// Set memory boundaries
				if (iMemory > T.GetSize() + iMult * 8 + iBytesUsedByVectors + _iNumOfThreads * Utilities::sBitArray(iAmountOfSpecies).sizeInBytes()) {
					iSoftMaxSizeOfInputVecs = static_cast<uint64_t>(0.9 * (iMemory - T.GetSize() - iMult * sizeof(double) - iMult * sizeof(uint64_t) - iBytesUsedByVectors - _iNumOfThreads * Utilities::sBitArray(iAmountOfSpecies).sizeInBytes()));
				}
				else {
					cerr << "ERROR: Not enough Memory given, try to download more RAM. Setting to 1GB..." << endl;
					//iSoftMaxSizeOfInputVecs = 1024ull * 1024ull * 1ull / (sizeof(tuple<uint64_t, uint64_t>)*_iNumOfThreads);
					iSoftMaxSizeOfInputVecs = static_cast<uint64_t>(0.9 * 1024ull * 1024ull * 1024ull); // 1024ull * 1024ull * 1024ull
				}

				unordered_map<uint64_t, Utilities::rangeContainer> vInputMap;

				const bool& bReadIDsAreInteresting = fOutFile != "";

				unique_ptr<float[]> vReadIDtoTaxID; // Array of #species times #reads with scores as values
				auto getVecOfScored = [&iAmountOfSpecies, &mIdxToTax](const unique_ptr<float[]>& vReadIDtoGenID, const uint64_t& iIdx) {
					try {
						vector<tuple<readIDType, float, double>> output;
						for (uint32_t i = 1; i < iAmountOfSpecies; ++i) {
							if (vReadIDtoGenID[iIdx*iAmountOfSpecies + i] > 0.f) {
								output.push_back(make_tuple(Utilities::checkIfInMap(mIdxToTax,i)->second, vReadIDtoGenID[iIdx*iAmountOfSpecies + i], 0.0));
							}
						}
						return output;
					}
					catch (...) {
						throw;
					}
				};

				list<pair<string, uint32_t>> vReadNameAndLength;

				uint64_t iNumberOfkMersInInput = 0;

				vector<string> vInputFiles = Utilities::gatherFilesFromPath(fInFile);

				// allow multiple input files
				for (const auto& inFile : vInputFiles) {

					string fileName = "";
					if (vInputFiles.size() > 1) { // get file name without path and ending
						const auto& vRawNameSplit = Utilities::split(inFile.substr(fInFile.size(), inFile.size()), '.');
						if (vRawNameSplit.size() == 1) {
							fileName = vRawNameSplit[0];
						}
						else {
							for (size_t rawC = 0; rawC < vRawNameSplit.size() - 1; ++rawC) {
								fileName += vRawNameSplit[rawC] + ".";
							}
							fileName.pop_back();
						}
					}

					// see if input is gziped or not and if it's a fasta or fastq file
					bool isGzipped = (inFile[inFile.length() - 3] == '.' && inFile[inFile.length() - 2] == 'g' && inFile[inFile.length() - 1] == 'z');
					bool bIsGood = false, bIsFasta = false;

					unique_ptr<ifstream> fast_q_a_File;
					unique_ptr<igzstream> fast_q_a_File_gz;
					uint64_t iFileLength = 0;

					if (isGzipped) {
						fast_q_a_File_gz.reset(new igzstream(inFile.c_str()));
						bIsGood = fast_q_a_File_gz->good();
						char cLessOrAt = static_cast<char>(fast_q_a_File_gz->peek());
						if (cLessOrAt == '>') {
							bIsFasta = true;
						}
						else {
							if (cLessOrAt == '@') {
								bIsFasta = false;
							}
							else {
								throw runtime_error("Input does not start with @ or >.");
							}
						}

						fast_q_a_File_gz->exceptions(std::ios_base::badbit);
					}
					else {
						fast_q_a_File.reset(new ifstream(inFile));
						bIsGood = fast_q_a_File->good();

						char cLessOrAt = static_cast<char>(fast_q_a_File->peek());
						if (cLessOrAt == '>') {
							bIsFasta = true;
						}
						else {
							if (cLessOrAt == '@') {
								bIsFasta = false;
							}
							else {
								throw runtime_error("Input does not start with @ or >.");
							}
						}

						fast_q_a_File->seekg(0, fast_q_a_File->end);
						iFileLength = fast_q_a_File->tellg();
						fast_q_a_File->seekg(0, fast_q_a_File->beg);
					}

					ofstream fOut;
					if (bReadIDsAreInteresting) {
						fOut.open((vInputFiles.size() > 1) ? fOutFile + fileName + ((bHumanReadable) ? ".rtt" : ".json") : fOutFile); // in case of multiple input files, specify only beginning of the output and the rest will be appended
						if (fOut) {
							if (bHumanReadable) {
								fOut << "#Read number\tSpecifier from input file\tMatched taxa\tNames\tScores{relative,k-mer}" << endl;
							}
							else {
								fOut << "[" << endl;
							}
						}
						else {
							throw runtime_error("Readwise output file could not be created!");
						}
					}
					

					unique_ptr<strTransfer> transferBetweenRuns(new strTransfer);
					vector<tuple<readIDType, float, double>> vSavedScores;
					readIDType iReadIDofSavedScores = 0;

					// read input
					while (bIsGood) {
						auto start = std::chrono::high_resolution_clock::now();
						if (bIsFasta) {
							if (isGzipped) {
								iNumberOfkMersInInput += readFasta_partialSort(*fast_q_a_File_gz, vInputMap, vReadNameAndLength, iSoftMaxSizeOfInputVecs, iAmountOfSpecies, bSpaced, iFileLength, bReadIDsAreInteresting, transferBetweenRuns, T);
							}
							else {
								iNumberOfkMersInInput += readFasta_partialSort(*fast_q_a_File, vInputMap, vReadNameAndLength, iSoftMaxSizeOfInputVecs, iAmountOfSpecies, bSpaced, iFileLength, bReadIDsAreInteresting, transferBetweenRuns, T);
							}

							iNumOfReads = transferBetweenRuns->vReadIDs.size();
						}
						else {
							if (isGzipped) {
								iNumberOfkMersInInput += readFastq_partialSort(*fast_q_a_File_gz, vInputMap, vReadNameAndLength, iSoftMaxSizeOfInputVecs, iAmountOfSpecies, bSpaced, iFileLength, bReadIDsAreInteresting, transferBetweenRuns, T);
							}
							else {
								iNumberOfkMersInInput += readFastq_partialSort(*fast_q_a_File, vInputMap, vReadNameAndLength, iSoftMaxSizeOfInputVecs, iAmountOfSpecies, bSpaced, iFileLength, bReadIDsAreInteresting, transferBetweenRuns, T);
							}

							iNumOfReads = transferBetweenRuns->vReadIDs.size();
						}

						// remove singletons
						/*for (auto it = vInput.begin(); it != vInput.end(); ) {
							if (it->second.kMers.size() == 1) {
								vInput.erase(it++);
							}
							else {
								++it;
							}
						}*/

						// sort suffices for each range in parallel
						vector<pair<uint64_t, Utilities::rangeContainer>> vInputVec;

						vInputVec.reserve(vInputMap.size());
						for (auto it = vInputMap.begin(); it != vInputMap.end();) {
							vInputVec.push_back(*it);
							vInputMap.erase(it++);
						}
						vInputMap.clear();
						sort(vInputVec.begin(), vInputVec.end(), [](const pair<uint64_t, Utilities::rangeContainer>& p1, const pair<uint64_t, Utilities::rangeContainer>& p2) { return p1.first < p2.first; });

#pragma omp parallel 
						{
							size_t iParallelCount = 0;
							int32_t iThreadID = omp_get_thread_num();
							int32_t iNumOfThreads = omp_get_num_threads();
							for (auto it = vInputVec.begin(); it != vInputVec.end(); ++it, ++iParallelCount) {
								if (static_cast<int32_t>(iParallelCount%iNumOfThreads) != iThreadID) {
									continue;
								}
								if (_iMinK <= 6) {
									sort(it->second.kMers_ST6.begin(), it->second.kMers_ST6.end(), [](const pair<uint64_t, readIDType>& a, const pair<uint64_t, readIDType>& b) { return a < b; });
								}
								else {
									sort(it->second.kMers_GT6.begin(), it->second.kMers_GT6.end(), [](const pair<uint32_t, readIDType>& a, const pair<uint32_t, readIDType>& b) { return a < b; });
								}
							}
						}
					
						auto end = std::chrono::high_resolution_clock::now();
						iTimeFastq += chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();


						if (bReadIDsAreInteresting) {
							// This holds the mapping read ID -> Genus ID
							if (iNumOfReads <= iNumOfReadsOld) {
								for (uint64_t i = 0; i < iNumOfReads*iAmountOfSpecies; ++i) {
									vReadIDtoTaxID[i] = 0.f;
								}
							}
							else {
								vReadIDtoTaxID.reset(new float[iNumOfReads*iAmountOfSpecies]);

								for (uint64_t i = 0; i < iNumOfReads*iAmountOfSpecies; ++i) {
									vReadIDtoTaxID[i] = 0.f;
								}
							}
						}
						else {
							iNumOfReadsSum += transferBetweenRuns->iNumOfNewReads;
						}

						// now compare with index
						start = std::chrono::high_resolution_clock::now();
						if (bReadIDsAreInteresting) {
							if (bPartitioned) {
								compareWithDatabase(vInputVec, vLibParted_p, vCount_all, vCount_unique, vReadIDtoTaxID, iAmountOfSpecies, transferBetweenRuns->mReadIDToArrayIdx);
							}
							else {
								compareWithDatabase(vInputVec, vLib, vCount_all, vCount_unique, vReadIDtoTaxID, iAmountOfSpecies, mTaxToIdx, transferBetweenRuns->mReadIDToArrayIdx);
							}
						}
						else {
							if (bPartitioned) {
								createProfile(vInputVec, vLibParted_p, vCount_all, vCount_unique, iAmountOfSpecies);
							}
							else {
								createProfile(vInputVec, vLib, vCount_all, vCount_unique, iAmountOfSpecies, mTaxToIdx);
							}
						}
						end = std::chrono::high_resolution_clock::now();
						iTimeCompare += chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

						// save results
						if (bReadIDsAreInteresting) {
							if (transferBetweenRuns->addTail) {
								// last read is not yet finished

								uint64_t i = 0;
								// check if there is still some unfinished read which is now complete
								if (vSavedScores.size()) {
									if (transferBetweenRuns->lastLine.second != iReadIDofSavedScores) {
										auto lastScoreVec = getVecOfScored(vReadIDtoTaxID, Utilities::checkIfInMap(transferBetweenRuns->mReadIDToArrayIdx, iReadIDofSavedScores)->second);
										vSavedScores.insert(vSavedScores.end(), lastScoreVec.cbegin(), lastScoreVec.cend());
										lastScoreVec.clear();
										sort(vSavedScores.begin(), vSavedScores.end(), [](const tuple<readIDType, float, double>& p1, const tuple<readIDType, float, double>& p2) { return get<0>(p1) < get<0>(p2); });
										auto seen = vSavedScores[0];
										for (auto it = vSavedScores.begin() + 1; it != vSavedScores.end(); ++it) {
											if (get<0>(*it) != get<0>(seen)) {
												lastScoreVec.push_back(seen);
												seen = *it;
											}
											else {
												get<1>(seen) += get<1>(*it);
											}
										}
										lastScoreVec.push_back(seen);
										vSavedScores.swap(lastScoreVec);
										scoringFunc(move(vSavedScores), (i++) + iNumOfReadsSum, vReadNameAndLength.front(), mFrequencies, mOrganisms, move(fOut));
										vSavedScores.clear();

										transferBetweenRuns->vReadIDs.erase(find(transferBetweenRuns->vReadIDs.begin(), transferBetweenRuns->vReadIDs.end(), iReadIDofSavedScores));
										transferBetweenRuns->mReadIDToArrayIdx.erase(iReadIDofSavedScores);
										vReadNameAndLength.pop_front();
									}
								}

								// save the score of the not yet finished
								auto resultOfUnfinished = getVecOfScored(vReadIDtoTaxID, Utilities::checkIfInMap(transferBetweenRuns->mReadIDToArrayIdx, transferBetweenRuns->lastLine.second)->second);
								if (resultOfUnfinished.size()) {
									vSavedScores.insert(vSavedScores.end(), resultOfUnfinished.cbegin(), resultOfUnfinished.cend());
									resultOfUnfinished.clear();
									sort(vSavedScores.begin(), vSavedScores.end(), [](const tuple<readIDType, float, double>& p1, const tuple<readIDType, float, double>& p2) { return get<0>(p1) < get<0>(p2); });
									auto seen = vSavedScores[0];
									for (auto it = vSavedScores.begin() + 1; it != vSavedScores.end(); ++it) {
										if (get<0>(*it) != get<0>(seen)) {
											resultOfUnfinished.push_back(seen);
											seen = *it;
										}
										else {
											get<1>(seen) += get<1>(*it);
										}
									}
									resultOfUnfinished.push_back(seen);
									vSavedScores.swap(resultOfUnfinished);
								}

								// save the finished ones
								for (; i < iNumOfReads - 1 && vReadNameAndLength.size(); ++i) {
									auto tempVec = getVecOfScored(vReadIDtoTaxID, Utilities::checkIfInMap(transferBetweenRuns->mReadIDToArrayIdx, transferBetweenRuns->vReadIDs.front())->second);
									if (tempVec.size()) {
										scoringFunc(move(tempVec), i + iNumOfReadsSum, vReadNameAndLength.front(), mFrequencies, mOrganisms, move(fOut));
									}
									else {
										if (bHumanReadable) {
											fOut << iNumOfReadsSum + i << "\t" << vReadNameAndLength.front().first << "\t-\t-" << endl;
										}
										else {
											if (iNumOfReadsSum + i == 0) {
												fOut << "{" << endl;
											}
											else {
												fOut << "," << endl << "{" << endl;
											}
											fOut << "\t\"Read number\": " << iNumOfReadsSum + i << "," << endl << "\t\"Specifier from input file\": \"" + vReadNameAndLength.front().first + "\"," << endl << "\t\"Matched taxa\": [" << endl << "\t]" << endl << "}";
										}
									}
									auto tempID = transferBetweenRuns->vReadIDs.front();
									transferBetweenRuns->vReadIDs.pop_front();
									transferBetweenRuns->mReadIDToArrayIdx.erase(tempID);
									vReadNameAndLength.pop_front();
								}


								iReadIDofSavedScores = transferBetweenRuns->iCurrentReadID;
								transferBetweenRuns->mReadIDToArrayIdx[iReadIDofSavedScores] = 0;

								iNumOfReadsSum += iNumOfReads - 1;
								iNumOfReadsOld = (iNumOfReads - 1 < iNumOfReadsOld) ? iNumOfReadsOld : iNumOfReads - 1;
							}
							else {
								// reads finished

								// write down the saved one
								uint64_t i = 0;
								if (vSavedScores.size()) {
									auto resultOfFinished = getVecOfScored(vReadIDtoTaxID, 0);
									vSavedScores.insert(vSavedScores.end(), resultOfFinished.cbegin(), resultOfFinished.cend());
									resultOfFinished.clear();
									sort(vSavedScores.begin(), vSavedScores.end(), [](const tuple<readIDType, float, double>& p1, const tuple<readIDType, float, double>& p2) { return get<0>(p1) < get<0>(p2); });
									auto seen = vSavedScores[0];
									for (auto it = vSavedScores.begin() + 1; it != vSavedScores.end(); ++it) {
										if (get<0>(*it) != get<0>(seen)) {
											resultOfFinished.push_back(seen);
											seen = *it;
										}
										else {
											get<1>(seen) += get<1>(*it);
										}
									}
									resultOfFinished.push_back(seen);
									vSavedScores.swap(resultOfFinished);

									scoringFunc(move(vSavedScores), iNumOfReadsSum, vReadNameAndLength.front(), mFrequencies, mOrganisms, move(fOut));
									i = 1;
									transferBetweenRuns->vReadIDs.erase(find(transferBetweenRuns->vReadIDs.begin(), transferBetweenRuns->vReadIDs.end(), iReadIDofSavedScores));
									transferBetweenRuns->mReadIDToArrayIdx.erase(iReadIDofSavedScores);
									vReadNameAndLength.pop_front();
									vSavedScores.clear();
								}

								// and now the regular ones
								for (; i < iNumOfReads; ++i) {
									auto tempVec = getVecOfScored(vReadIDtoTaxID, i);
									if (tempVec.size()) {
										scoringFunc(move(tempVec), i + iNumOfReadsSum, vReadNameAndLength.front(), mFrequencies, mOrganisms, move(fOut));
									}
									else {
										if (bHumanReadable) {
											fOut << iNumOfReadsSum + i << "\t" << vReadNameAndLength.front().first << "\t-\t-" << endl;
										}
										else {
											if (iNumOfReadsSum + i == 0) {
												fOut << "{" << endl;
											}
											else {
												fOut << "," << endl << "{" << endl;
											}
											fOut << "\t\"Read number\": " << iNumOfReadsSum + i << "," << endl << "\t\"Specifier from input file\": \"" + vReadNameAndLength.front().first + "\"," << endl << "\t\"Matched taxa\": [" << endl << "\t]" << endl << "}";
										}
									}
									auto tempID = transferBetweenRuns->vReadIDs.front();
									transferBetweenRuns->vReadIDs.pop_front();
									transferBetweenRuns->mReadIDToArrayIdx.erase(tempID);
									vReadNameAndLength.pop_front();
								}
								iNumOfReadsSum += iNumOfReads;
								iNumOfReadsOld = iNumOfReads;
								vReadNameAndLength.clear();
							}
						}


						vInputVec.clear();

						if (isGzipped) {
							bIsGood = fast_q_a_File_gz->good();
						}
						else {
							bIsGood = fast_q_a_File->good();
						}

						//iterate until no dna is left
					}

					// if json is the output format for readToTaxa, end it with a ]
					if (bReadIDsAreInteresting && !bHumanReadable) {
						fOut << endl << "]";
					}

					// get profiling results
					vector<uint64_t> vSumOfUniquekMers(_iNumOfK);
					vector<double> vSumOfNonUniques(_iNumOfK);
					vector<tuple<string, vector<pair<double, uint64_t>>, uint32_t>> vOut(iAmountOfSpecies, tuple<string, vector<pair<double, uint64_t>>, uint32_t>("", vector<pair<double, uint64_t>>(_iNumOfK), 0));
					for (uint32_t iSpecIdx = 1; iSpecIdx < iAmountOfSpecies; ++iSpecIdx) {
						vector<pair<double, uint64_t>> vTemp(_iNumOfK);
						for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
							const uint64_t& iTempScore = vCount_unique[iSpecIdx*uint64_t(_iNumOfK) + ikMerlength];
							vSumOfUniquekMers[ikMerlength] += iTempScore;
							vSumOfNonUniques[ikMerlength] += vCount_all[iSpecIdx*uint64_t(_iNumOfK) + ikMerlength];
							vTemp[ikMerlength] = make_pair(vCount_all[iSpecIdx*uint64_t(_iNumOfK) + ikMerlength], iTempScore);
						}
						vOut[iSpecIdx] = make_tuple(Utilities::checkIfInMap(mOrganisms, Utilities::checkIfInMap(mIdxToTax, iSpecIdx)->second)->second, vTemp, Utilities::checkIfInMap(mIdxToTax, iSpecIdx)->second);
					}
					sort(vOut.begin(), vOut.end(), [](const tuple<string, vector<pair<double, uint64_t>>, uint32_t>& a, const tuple<string, vector<pair<double, uint64_t>>, uint32_t>& b) {
						for (uint64_t i = 0; i < get<1>(a).size(); ++i) {
							if (get<1>(a)[i].second == get<1>(b)[i].second) {
								continue;
							}
							else {
								return get<1>(a)[i].second > get<1>(b)[i].second;
							}
						}
						return false;
					});

					// save to file(s)
					ofstream tableFileStream((vInputFiles.size() > 1) ? fTableFile + fileName + ".csv" : fTableFile);
					//auto orgBuf = cout.rdbuf();
					if (fTableFile != "") {
						//cout.rdbuf(tableFileStream.rdbuf());

						if (bHumanReadable) {
							// short version: taxID,Name,Unique Percentage of highest k,Non-unique Percentage of highest k\n
							bool bBreakOut = false;
							tableFileStream << "#tax ID,Name,Unique rel. freq.,Non-unique rel. freq.\n";
							for (const auto& entry : vOut) {
								if (get<1>(entry)[_iNumOfK - 1].first > 0 && !bBreakOut) {
									tableFileStream << get<2>(entry) << "," << get<0>(entry);
									if (get<1>(entry)[0].second == 0) {
										tableFileStream << "," << 0.0 << "%";
										bBreakOut = true;
									}
									else {
										tableFileStream << "," << static_cast<double>(get<1>(entry)[0].second) / vSumOfUniquekMers[0] * 100.0 << "%";
									}
									if (get<1>(entry)[0].first == 0) {
										tableFileStream << "," << 0.0 << "%";
									}
									else {
										tableFileStream << "," << static_cast<double>(get<1>(entry)[0].first) / vSumOfNonUniques[0] * 100.0 << "%";
									}
									tableFileStream << endl;
								}
							}
						}
						else {
							// long version: taxID,Name,Unique Counts,Unique rel. freq. x in 0.x,Non-unique Counts,Non-unique rel. freq. x in 0.x\n
							tableFileStream << "#taxID,Name";
							for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
								tableFileStream << "," << "Unique counts k=" << 12 - ikMerlength;
							}
							for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
								tableFileStream << "," << "Unique rel. freq. k=" << 12 - ikMerlength;
							}
							for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
								tableFileStream << "," << "Non-unique counts k=" << 12 - ikMerlength;
							}
							for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
								tableFileStream << "," << "Non-unique rel. freq. k=" << 12 - ikMerlength;
							}
							tableFileStream << endl;
							for (const auto& entry : vOut) {
								if (get<1>(entry)[_iNumOfK - 1].first > 0) {
									tableFileStream << get<2>(entry) << "," << get<0>(entry);
									for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
										tableFileStream << "," << get<1>(entry)[ikMerlength].second;
									}
									for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
										if (get<1>(entry)[ikMerlength].second == 0) {
											tableFileStream << "," << 0.0;
										}
										else {
											tableFileStream << "," << static_cast<double>(get<1>(entry)[ikMerlength].second) / vSumOfUniquekMers[ikMerlength];
										}
									}
									for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
										tableFileStream << "," << get<1>(entry)[ikMerlength].first;
									}
									for (int32_t ikMerlength = 0; ikMerlength < _iNumOfK; ++ikMerlength) {
										if (bSpaced) {
											tableFileStream << "," << static_cast<double>(get<1>(entry)[ikMerlength].first) / vSumOfNonUniques[ikMerlength];
										}
										else {
											//tableFileStream << "," << static_cast<double>(get<1>(entry)[ikMerlength]) / (iNumberOfkMersInInput - aNonUniqueHits[ikMerlength] - (_iMaxK - _iMinK - ikMerlength) * 6 * iNumOfReadsSum);
											tableFileStream << "," << static_cast<double>(get<1>(entry)[ikMerlength].first) / vSumOfNonUniques[ikMerlength];
										}
									}
									tableFileStream << endl;
								}
							}
						}
					}
					/*if (fTableFile != "") {
						cout.rdbuf(orgBuf);
					}*/
					if (_bVerbose) {
						cout << "OUT: Number of kMers in Input: " << iNumberOfkMersInInput << endl;
						cout << "OUT: Number of uniques:";
						for (int32_t j = 0; j < _iNumOfK; ++j) {
							cout << " " << vSumOfUniquekMers[j];
						}
						cout << endl;
						cout << "OUT: Time fastq: " << iTimeFastq << " ns" << endl;
						cout << "OUT: Time compare: " << iTimeCompare << " ns" << endl;
					}

					iNumOfReadsSum = 0;
					iNumberOfkMersInInput = 0;
					iTimeFastq = 0;
					iTimeCompare = 0;
					vReadNameAndLength.clear();

					for (uint64_t i = 0; i < iMult; ++i) {
						vCount_all[i] = 0.;
						vCount_unique[i] = 0;
					}
				}
			}
			catch (...) {
				throw;
			}
		}
		/*
		void testC2V_1();
		void testC2V_2();
		void testC2V_3();
		void testC2V_4();
		void testC2V_5();
		void testC2V_6();
		void testC2V_7();
		void testC2V_8();
		void testC2V_9();*/
	};
}