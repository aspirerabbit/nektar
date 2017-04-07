///////////////////////////////////////////////////////////////////////////////
//
// File: NekFactory.hpp
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
// Description: Factory pattern class for Nektar
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NEKTAR_LIBUTILITIES_BASICUTILS_NEKFACTORY
#define NEKTAR_LIBUTILITIES_BASICUTILS_NEKFACTORY

// Primary definition and generator for specialised object factories.
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <map>
#include <string>

#include <LibUtilities/BasicUtils/ErrorUtil.hpp>

namespace Nektar
{
namespace LibUtilities
{

// Generate parameter typenames with default type of 'none'
typedef boost::unique_lock<boost::shared_mutex> WriteLock;
typedef boost::shared_lock<boost::shared_mutex> ReadLock;

/**
 * @class NekFactory
 \brief Provides a generic Factory class.
 *
 * Implements a generic object factory. Class-types which use a
 * potentially arbitrary number of parameters may be used with
 * specialised forms of the NekFactory. An upper
 * limit on the number of parameters is imposed by the MAX_PARAM
 * preprocessor definition in the NekFactory.hpp file. The
 * specialisations are generated at compile type using Boost
 * preprocessor by through repeated inclusion of the NekFactory.hpp
 * file.
 *
 * To allow a class to be instantiated by the factory, the
 * following are required in each class definition (in the case of
 * a single parameter):
 * \code
 *   static [baseclass]* create([paramtype1] &P) {
 *      return new [derivedclass](P);
 *   }
 *   static std::string className;
 * \endcode
 * and outside the class definition in the implementation:
 * \code
 *   std::string [derivedclass]::className
 *      = Factory<std::string,[baseclass],[paramtype1]>::
 *          RegisterCreatorFunction("[derivedclass]",
 *                              [derivedclass]::create,"Description");
 * \endcode
 * The assignment of the static variable className is done through the
 * call to RegisterCreatorFunction, which registers the class with the
 * factory prior to the start of the main() routine.
 *
 * To create an instance of a derived class, for instance:
 * \code
 *   [baseclass]* var_name =
 *      Factory<std::string,[baseclass],[paramtype1]>
 *              ::CreateInstance("[derivedclass]",Param1);
 * \endcode
 */
template <typename tKey,        // reference tag (e.g. string, int)
          typename tBase,       // base class
          typename... tParam>
class NekFactory
{
public:
    /// Description datatype
    typedef std::string tDescription;
    /// Comparison predicator of key
    typedef std::less<tKey> tPredicator;
    /// Shared pointer to an object of baseclass type.
    typedef boost::shared_ptr<tBase> tBaseSharedPtr;
    /// CreatorFunction type which takes parameter and returns base class shared
    /// pointer.
    typedef tBaseSharedPtr (*CreatorFunction) (tParam...);

    /// Define a struct to hold the information about a module.
    struct ModuleEntry
    {
        ModuleEntry(CreatorFunction pFunc, const tDescription pDesc)
            : m_func(pFunc),
              m_desc(pDesc)
        {
        }

        /// Function used to create instance of class.
        CreatorFunction m_func;
        /// Description of class for use in listing available classes.
        tDescription m_desc;
    };

    /// Factory map between key and module data.
    typedef std::map<tKey, ModuleEntry, tPredicator> TMapFactory;
    /// Iterator for factory map
    typedef typename TMapFactory::iterator TMapFactoryIterator;

public:
    NekFactory() : m_mutex() {}

    /**
     * @brief Create an instance of the class referred to by \c idKey.
     *
     * Searches the factory's map for the given key and returns a shared
     * base class pointer to a new instance of the associated class.
     * @param   idKey           Key of class to create.
     * @param   x               Parameter to pass to class constructor.
     * @returns                 Base class pointer to new instance.
     */
    tBaseSharedPtr CreateInstance(tKey idKey, tParam... args)
    {
        ReadLock vReadLock(m_mutex);

        // Now try and find the key in the map.
        TMapFactoryIterator it = getMapFactory()->find(idKey);

        // If successful, check the CreatorFunction is defined and
        // create a new instance of the class.
        if (it != getMapFactory()->end())
        {
            ModuleEntry *tmp = &(it->second);
            vReadLock.unlock();

            if (tmp->m_func)
            {
                try
                {
                    return tmp->m_func(args...);
                }
                catch (const std::string& s)
                {
                    std::stringstream errstr;
                    errstr << "Unable to create module: " << idKey << "\n";
                    errstr << s;
                    ASSERTL0(false, errstr.str());
                }
            }
        }

        // If we get this far, the key doesn't exist, so throw an error.
        std::stringstream errstr;
        errstr << "No such module: " << idKey << std::endl;
        PrintAvailableClasses(errstr);
        ASSERTL0(false, errstr.str());
        return tBaseSharedPtr();
    }


    /**
     * @brief Register a class with the factory.
     *
     * This function is called by each class in a static context (prior
     * to the execution of main()) and creates an entry for the class
     * in the factory's map.
     * @param   idKey           Key used to reference the class.
     * @param   classCreator    Function to call to create an instance
     *                          of this class.
     * @param   pDesc           Optional description of class.
     * @returns                 The given key \c idKey.
     */
    tKey RegisterCreatorFunction(tKey idKey, CreatorFunction classCreator,
                                 tDescription pDesc = "")
    {
        WriteLock vWriteLock(m_mutex);

        ModuleEntry e(classCreator, pDesc);
        getMapFactory()->insert(std::pair<tKey,ModuleEntry>(idKey, e));
        return idKey;
    }


    /**
     * @brief Checks if a particular module is available.
     */
    bool ModuleExists(tKey idKey)
    {
        ReadLock vReadLock(m_mutex);

        // Now try and find the key in the map.
        TMapFactoryIterator it = getMapFactory()->find(idKey);

        if (it != getMapFactory()->end())
        {
            return true;
        }
        return false;
    }


    /**
     * @brief Prints the available classes to stdout.
     */
    void PrintAvailableClasses(std::ostream& pOut = std::cout)
    {
        ReadLock vReadLock(m_mutex);

        pOut << std::endl << "Available classes: " << std::endl;
        TMapFactoryIterator it;
        for (it = getMapFactory()->begin(); it != getMapFactory()->end(); ++it)
        {
            pOut << std::endl << "Available classes: " << std::endl;
            TMapFactoryIterator it;
            for (it = getMapFactory()->begin(); it != getMapFactory()->end(); ++it)
            {
                pOut << "  " << it->first;
                if (it->second.m_desc != "")
                {
                    pOut << ":" << std::endl << "    "
                         << it->second.m_desc << std::endl;
                }
                else
                {
                    pOut << std::endl;
                }
            }
        }
    }


    /**
     * @brief Retrieves a key, given a description
     */
    tKey GetKey(tDescription pDesc)
    {
        ReadLock vReadLock(m_mutex);

        TMapFactoryIterator it;
        for (it = getMapFactory()->begin(); it != getMapFactory()->end(); ++it)
        {
            if (it->second.m_desc == pDesc)
            {
                return it->first;
            }
        }
        std::string errstr = "Module '"
            + boost::lexical_cast<std::string>(pDesc)
            + "' is not known.";
        ASSERTL0(false, errstr);
    }


    /**
     * @brief Returns the description of a class
     */
    std::string GetClassDescription(tKey idKey)
    {
        ReadLock vReadLock(m_mutex);

        // Now try and find the key in the map.
        TMapFactoryIterator it = getMapFactory()->find(idKey);

        std::stringstream errstr;
        errstr << "No such module: " << idKey << std::endl;
        ASSERTL0 (it != getMapFactory()->end(), errstr.str());
        return it->second.m_desc;
    }

protected:
    /**
     * @brief Ensure the factory's map is created.
     * @returns                 The factory's map.
     */
    TMapFactory* getMapFactory() 
    {
        return &mMapFactory;
    }

private:
    NekFactory(const NekFactory& rhs);
    NekFactory& operator=(const NekFactory& rhs);

    TMapFactory mMapFactory;

    boost::shared_mutex m_mutex;

};

}
}

#endif
