#ifndef _CCNRP_CONFIG_H
#define _CCNRP_CONFIG_H

#include <list>
#include <map>

#define CCNRP_CONFIG_LINE_LEN           1024

class CcnrpConfig
{
  // Enums and Typedefs
  typedef std::list<std::string> StringList_t;
  typedef StringList_t::iterator StringListIter_t;

  typedef std::map<std::string, StringList_t> ConfigMap_t;
  typedef ConfigMap_t::iterator               ConfigMapIter_t;

  // Member Variables
  private:
    char *m_szFile;
    const char *m_szError;
    ConfigMap_t m_oConfigMap;

    static CcnrpConfig s_oInstance;

  // Methods
  private:
    CcnrpConfig(CcnrpConfig &p_oRHS);
    CcnrpConfig &operator=(CcnrpConfig &p_oRHS);

  public:
    CcnrpConfig();
    virtual ~CcnrpConfig();

    static CcnrpConfig &getInstance();

    void clear();
    int load(char *p_szFile);
    int load(const char *p_szFile);

    const char *getValue(char *p_szKey);
    const char *getValue(const char *p_szKey);

    const char *getError();

  private:
    char *_trim(char *p_szLine, int p_iMaxLen);
    void _chomp(char *p_szLine, int p_iMaxLen);
};

#endif
