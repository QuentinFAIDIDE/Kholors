#include "GitWrapper.h"

GitWrapper::GitWrapper()
{
    git_libgit2_init();
    libgitRepo = nullptr;
}

GitWrapper::~GitWrapper()
{
    if (libgitRepo != nullptr)
    {
        git_repository_free(libgitRepo);
    }
    git_libgit2_shutdown();
}

void GitWrapper::setWorkingDirectory(std::string path)
{
    repositoryPath = path;

    if (libgitRepo != nullptr)
    {
        git_repository_free(libgitRepo);
    }
}

std::string GitWrapper::init()
{
    if (repositoryPath == "")
    {
        return "No repository set";
    }

    int ret = git_repository_init(&libgitRepo, repositoryPath.c_str(), 0);
    if (ret < 0)
    {
        return git_error_last()->message;
    }

    return "";
}

std::string GitWrapper::open()
{
    if (repositoryPath == "")
    {
        return "No repository set";
    }

    int error = git_repository_open(&libgitRepo, repositoryPath.c_str());

    if (error < 0)
    {
        return git_error_last()->message;
    }

    return "";
}

void GitWrapper::add(std::string filenameRelativeFromDirectory)
{
    if (repositoryPath == "")
    {
        return;
    }
}

void GitWrapper::commit(std::string commitMessage)
{
    if (repositoryPath == "")
    {
        return;
    }
    // TODO
}

std::string GitWrapper::getBranch()
{
    if (repositoryPath == "")
    {
        return "";
    }
    // TODO
    return "";
}
