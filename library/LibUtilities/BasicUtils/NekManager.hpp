///////////////////////////////////////////////////////////////////////////////
//
// File: NekManager.hpp
//
// For more information, please see: http://www.nektar.info
//
// The MIT License
//
// Copyright (c) 2006 Division of Applied Mathematics, Brown University (USA),
// Department of Aeronautics, Imperial College London (UK), and Scientific
// Computing and Imaging Institute, University of Utah (USA).
//
// License for the specific language governing rights and limitations under
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Description: 
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_LIB_UTILITIES_BASIC_UTILS_NEK_MANAGER_HPP
#define NEKTAR_LIB_UTILITIES_BASIC_UTILS_NEK_MANAGER_HPP

#include <map>

#include <boost/function.hpp>
#include <boost/call_traits.hpp>
#include <boost/concept_check.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include <boost/shared_ptr.hpp>
#include <LibUtilities/BasicUtils/ErrorUtil.hpp>
#include <LibUtilities/BasicUtils/Thread.h>

using namespace std;

namespace Nektar
{
    namespace LibUtilities
    {
        template <typename KeyType>
        struct defOpLessCreator
        {
            bool operator()(const KeyType &lhs, const KeyType &rhs) const
            {
                return lhs < rhs;
            }
        };

    	typedef boost::unique_lock<boost::shared_mutex> WriteLock;
    	typedef boost::shared_lock<boost::shared_mutex> ReadLock;

        template <typename KeyType, typename ValueT, typename opLessCreator = defOpLessCreator<KeyType> >
        class NekManager
        {
            public:
                BOOST_CLASS_REQUIRE(KeyType, boost, LessThanComparableConcept);

                typedef boost::shared_ptr<ValueT> ValueType;
                typedef boost::function<ValueType (const KeyType& key)> CreateFuncType;
                typedef std::map<KeyType, ValueType> ValueContainer;
                typedef boost::shared_ptr<ValueContainer> ValueContainerShPtr;
                typedef std::map<KeyType, CreateFuncType, opLessCreator> CreateFuncContainer;
                typedef std::map<std::string, boost::shared_ptr<ValueContainer> > ValueContainerPool;
                typedef boost::shared_ptr<bool> BoolSharedPtr;
                typedef std::map<std::string, BoolSharedPtr> FlagContainerPool;

                NekManager(std::string whichPool="") :
                    m_values(),
                    m_globalCreateFunc(),
                    m_keySpecificCreateFuncs()

                {
//                	std::cerr << "Called regular constructor" << std::endl;
                	Init(whichPool);
                }

//                NekManager(NekManager<KeyType, ValueType, opLessCreator> * rhs, unsigned int pThr) :
//                	m_values(),
//                    m_globalCreateFunc(rhs->m_globalCreateFunc),
//                    m_keySpecificCreateFuncs(rhs->m_keySpecificCreateFuncs)
//                {
//                	std::cerr << "Called pointer copy constructor with rsh: " << &rhs << " from thr: " << pThr << std::endl;
//                	Init();
//                }
//

                NekManager(CreateFuncType f, std::string whichPool="") :
                    m_values(),
                    m_globalCreateFunc(f),
                    m_keySpecificCreateFuncs()
                {
//                	std::cerr << "Called regular createfunctype constructor" << std::endl;
                	Init(whichPool);
                }

                void Init(std::string& whichPool = "")
                {
                	if (!whichPool.empty())
                	{
                		Nektar::Thread::ThreadManagerSharedPtr vThrMan = Nektar::Thread::ThreadManager::GetInstance();
                		ASSERTL1(vThrMan, "Cannot construct a NekManager with a pool before threading is inititialised");
                		unsigned int vThr = vThrMan->GetWorkerNum();
                		whichPool.append(boost::lexical_cast<std::string>(vThr));

            			ReadLock v_rlock(m_mutex); // reading static members
                		typename ValueContainerPool::iterator iter = m_ValueContainerPool.find(whichPool);
                		if (iter != m_ValueContainerPool.end())
                		{
                			m_values = iter->second;
                			m_managementEnabled = m_managementEnabledContainerPool[whichPool];
                		}
                		else
                		{
                			v_rlock.unlock();
                			// now writing static members.  Apparently upgrade_lock has less desirable properties
                			// than just dropping read lock, grabbing write lock.
                			// write will block until all reads are done, but reads cannot be acquired if write
                			// lock is blocking.  In this context writes are supposed to be rare.
                			WriteLock v_wlock(m_mutex);
                			m_values = ValueContainerShPtr(new ValueContainer);
                			m_ValueContainerPool[whichPool] = m_values;
                			if (m_managementEnabledContainerPool.find(whichPool) == m_managementEnabledContainerPool.end())
                			{
                				m_managementEnabledContainerPool[whichPool] = BoolSharedPtr(new bool(true));
                			}
                			m_managementEnabled = m_managementEnabledContainerPool[whichPool];
                		}

                	}
                	else
                	{
                		m_values = ValueContainerShPtr(new ValueContainer);
                		m_managementEnabled = BoolSharedPtr(new bool(true));
                	}
                }

                ~NekManager()
                {
                }

                /// Register the given function and associate it with the key.
                /// The return value is just to facilitate calling statically.
                bool RegisterCreator(typename boost::call_traits<KeyType>::const_reference key,
                                     const CreateFuncType& createFunc)
                {
                    m_keySpecificCreateFuncs[key] = createFunc;

                    return true;
                }

                /// Register the Global Create Function.
                /// The return value is just to facilitate calling statically.
                bool RegisterGlobalCreator(const CreateFuncType& createFunc)
                {
                    m_globalCreateFunc = createFunc;

                    return true;
                }

                bool AlreadyCreated(typename boost::call_traits<KeyType>::const_reference key)
                {
                    bool value = false;
                    typename ValueContainer::iterator found = m_values->find(key);
                    if( found != m_values->end() )
                    {
                        value = true;
                    }

                    return value;
                }

                ValueType operator[](typename boost::call_traits<KeyType>::const_reference key)
                {
//            		Nektar::Thread::ThreadManagerSharedPtr vThrMan = Nektar::Thread::ThreadManager::GetInstance();
//            		ASSERTL1(vThrMan, "Cannot construct a NekManager with a pool before threading is inititialised");
//            		unsigned int vThr = vThrMan->GetWorkerNum();

                    typename ValueContainer::iterator found = m_values->find(key);

                    if( found != m_values->end() )
                    {
//                    	std::cerr << "Thr: " << vThr << " found: " << ((*found).second).get() << std::endl;
                        return (*found).second;
                    }
                    else
                    {
                        // No object, create a new one.
                        CreateFuncType f = m_globalCreateFunc;
                        typename CreateFuncContainer::iterator keyFound = m_keySpecificCreateFuncs.find(key);
                        if( keyFound != m_keySpecificCreateFuncs.end() )
                        {
                            f = (*keyFound).second;
                        }

                        if( f )
                        {
                            ValueType v = f(key);
                            if (*m_managementEnabled)
                            {
                                (*m_values)[key] = v;
                            }
//                        	std::cerr << "Thr: " << vThr << " made: " << v.get() << std::endl;
                            return v;
                        }
                        else
                        {
                            std::string keyAsString = boost::lexical_cast<std::string>(key);
                            std::string message = std::string("No create func found for key ") + keyAsString;
                            NEKERROR(ErrorUtil::efatal, message.c_str());
                            static ValueType result;
                            return result;
                        }
                    }
                }

                void Clone(const NekManager<KeyType, ValueT, opLessCreator> &c)
                {
                	m_globalCreateFunc = c.m_globalCreateFunc;

                	m_keySpecificCreateFuncs.insert(c.m_keySpecificCreateFuncs.begin(),
                			c.m_keySpecificCreateFuncs.end());
                }

                static void ClearManager(std::string whichPool = "")
                {
                	Nektar::Thread::ThreadManagerSharedPtr vThrMan = Nektar::Thread::ThreadManager::GetInstance();
                    typename ValueContainerPool::iterator x;
                    if (!whichPool.empty())
                    {
                    	WriteLock v_wlock(m_mutex);

                    	ASSERTL1(vThrMan, "Cannot clear a NekManager with a pool before threading is inititialised");
                    	unsigned int vThr = vThrMan->GetWorkerNum();
                    	whichPool.append(boost::lexical_cast<std::string>(vThr));

                        x = m_ValueContainerPool.find(whichPool);
                        ASSERTL1(x != m_ValueContainerPool.end(),
                                "Could not find pool " + whichPool);
                        x->second->clear();
                    }
                    else
                    {
                    	WriteLock v_wlock(m_mutex);

                    	std::string vAppend;
                    	if (vThrMan)
                    	{
                        	unsigned int vThr = vThrMan->GetWorkerNum();
                        	vAppend.append(boost::lexical_cast<std::string>(vThr));
                    	}
                        for (x = m_ValueContainerPool.begin(); x != m_ValueContainerPool.end(); ++x)
                        {
                            if (x->first.rfind(vAppend) == x->first.length() - vAppend.length())
                            {
                            	x->second->clear();
                            }
                        }
                    }
                }

                static void EnableManagement(std::string whichPool = "")
                {
                    typename FlagContainerPool::iterator x;
                    if (!whichPool.empty())
                    {
                    	WriteLock v_wlock(m_mutex);

                    	Nektar::Thread::ThreadManagerSharedPtr vThrMan = Nektar::Thread::ThreadManager::GetInstance();
                    	ASSERTL1(vThrMan, "Cannot enable a NekManager with a pool before threading is inititialised");
                    	unsigned int vThr = vThrMan->GetWorkerNum();
                    	whichPool.append(boost::lexical_cast<std::string>(vThr));

                        x = m_managementEnabledContainerPool.find(whichPool);
                        if (x != m_managementEnabledContainerPool.end())
                        {
                            (*x->second) = true;
                        }
                        else
                        {
                            m_managementEnabledContainerPool[whichPool] = BoolSharedPtr(new bool(true));
                        }
                    }
                }

                static void DisableManagement(std::string whichPool = "")
                {
                    typename FlagContainerPool::iterator x;
                    if (!whichPool.empty())
                    {
                    	WriteLock v_wlock(m_mutex);

                    	Nektar::Thread::ThreadManagerSharedPtr vThrMan = Nektar::Thread::ThreadManager::GetInstance();
                    	ASSERTL1(vThrMan, "Cannot disable a NekManager with a pool before threading is inititialised");
                    	unsigned int vThr = vThrMan->GetWorkerNum();
                    	whichPool.append(boost::lexical_cast<std::string>(vThr));

                        x = m_managementEnabledContainerPool.find(whichPool);
                        if (x != m_managementEnabledContainerPool.end())
                        {
                            (*x->second) = false;
                        }
                        else
                        {
                            m_managementEnabledContainerPool[whichPool] = BoolSharedPtr(new bool(false));
                        }
                    }
                }

            private:
                NekManager<KeyType, ValueType, opLessCreator>& operator=(const NekManager<KeyType, ValueType, opLessCreator>& rhs);
                NekManager(const NekManager<KeyType, ValueType, opLessCreator>& rhs);

                ValueContainerShPtr m_values;
                BoolSharedPtr m_managementEnabled;
                static ValueContainerPool m_ValueContainerPool;
                static FlagContainerPool m_managementEnabledContainerPool;
                CreateFuncType m_globalCreateFunc;
                CreateFuncContainer m_keySpecificCreateFuncs;
                static boost::shared_mutex m_mutex;
        };
        template <typename KeyType, typename ValueT, typename opLessCreator>
        typename NekManager<KeyType, ValueT, opLessCreator>::ValueContainerPool NekManager<KeyType, ValueT, opLessCreator>::m_ValueContainerPool;
        template <typename KeyType, typename ValueT, typename opLessCreator>
        typename NekManager<KeyType, ValueT, opLessCreator>::FlagContainerPool NekManager<KeyType, ValueT, opLessCreator>::m_managementEnabledContainerPool;
        template <typename KeyType, typename ValueT, typename opLessCreator>
        typename boost::shared_mutex NekManager<KeyType, ValueT, opLessCreator>::m_mutex;
    }
}


#endif //NEKTAR_LIB_UTILITIES_BASIC_UTILS_NEK_MANAGER_HPP
