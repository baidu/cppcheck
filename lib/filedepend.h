#pragma once
#ifndef BCACPP_FILEDEPEND_H
#define BCACPP_FILEDEPEND_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>

typedef unsigned long long UINT64;
typedef bool (*fp_fileFilter)(const std::string &filename);
class CFileBase;
class CFolder;
class CCodeFile;

class CFileDependTable
{

public:
    CFileDependTable();
    ~CFileDependTable();
    // ��������·���������ļ�������
    bool create(
        const std::vector<std::string>& paths,
        const std::vector<std::string>& excludesPaths,
        const std::vector<std::string>& includePaths);

    CCodeFile* get_first_file();
    CFileBase* find_file(const std::string& filePath);
    void dump_file_depend_results();

    static std::string get_program_directory();
    static bool create_log_directory(std::string* pLogPath = NULL);
private:
    bool create_file_depend_tree(const std::vector<std::string> &paths,
        const std::vector<std::string> &includePaths,
        const std::vector<std::string>& excludesPaths);

    bool build_file_depend_tree(std::string &sPath, fp_fileFilter fp);

    void update_includes(const std::vector<std::string>& includePaths);
    bool build_table(CFolder* pRoot, const std::string& sPath, fp_fileFilter fp);
    void release_table();

    static unsigned char ReadChar(std::istream &istr, unsigned int bom);

    void GetIncludes(const std::string &fileName, std::vector<std::string> &strIncludes);
    void ParseIncludes(CCodeFile* pCode);
    CCodeFile* FindMatchedFile(CCodeFile* pFile, std::string sInclude);
    CCodeFile* FindShortestPath(const std::vector<CCodeFile*>& vecFiles, CCodeFile* pFile);
    std::size_t GetFileSize(const std::string& sPath);

    std::string getAbsolutePath(const std::string& path);
    
private:
    // �ļ��ṹ���ĸ��ڵ�
    CFolder* m_pRoot;
    CCodeFile* m_begin;
    CCodeFile* m_flag;
    // �ַ������ļ��ڵ��ӳ�䣬���ܴ���һ�Զ��������ļ�������
    std::multimap<std::string, CCodeFile*> m_fileDict;
    std::vector<CFolder*> m_includePaths;
};

enum EFileType { FT_NONE, FT_FOLDER, FT_CODE };

// �ļ��ڵ����
class CFileBase
{
public:
    void SetParent(CFolder* pParent);
    EFileType GetFileType();
    const std::string& GetName();
    CFolder* GetParent();
    std::string GetFullPath();

    virtual void SetIgnore(bool ignore) { m_bIgnore = ignore; }
    bool GetIgnore() const { return m_bIgnore; }
    virtual ~CFileBase();
protected:
    CFileBase();

    // ÿһ���ļ���Ӧ��Ψһ���
    unsigned int m_uID;
    // �ļ��л����ļ�����
    std::string m_name;
    // ���ڵ�ָ��
    CFolder* m_parent;
    EFileType m_fileType;

    bool m_bIgnore;
private:
    static unsigned int s_id;
};

// �ļ�����
class CFolder : public CFileBase
{
public:
    CFolder(const char* szName);
    virtual ~CFolder();

    void AddFile(CFileBase* pFile);
    void RemoveFile(const std::string& sName);
    CCodeFile*  AddCodeFile(const std::string& sName, std::size_t uSize, bool& bNew);
    CFolder* AddFolder(const std::string& sName, bool& bNew);
    CFileBase* find_file(const std::string& fileName);
    void Release();
    const std::vector<CFileBase*>& GetSubs();
    
    virtual void SetIgnore(bool ignore);
private:
    // �ӽڵ�ָ�뼯��
    std::vector<CFileBase*> m_subs;
};

// �����ļ���
class CCodeFile : public CFileBase
{
public:
    CCodeFile(const char* szName, std::size_t size);
    virtual ~CCodeFile();

    void set_next(CCodeFile* next);
    CCodeFile* get_next();

    std::size_t get_size() const;

    void add_depend_file(CCodeFile* pFile);
    std::vector<CCodeFile*>& get_depends();

    std::list<CCodeFile*>& get_all_depends();

    void add_expand_count();
    unsigned int get_expand_count() const { return m_nExpandCount; }
    bool is_expaned() const { return m_nExpandCount != 0; }
private:
    void fill_all_depends();
    static void expand_includes(CCodeFile* pCodeFile,
        std::list<CCodeFile*>& allDepends,
        std::set<CCodeFile*>& included);
private:
    // �ļ���С
    std::size_t m_size;
    // �����ļ��������ļ��б�
    std::vector<CCodeFile*> m_depends;
    // �����ļ����������ļ��б�
    std::vector<CCodeFile*> m_beDepends;

    std::list<CCodeFile*> m_allDepends;

    CCodeFile* m_next;

    unsigned int m_nExpandCount;
}; 
#endif // BCACPP_FILEDEPEND_H