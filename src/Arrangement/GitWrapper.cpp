#include "GitWrapper.h"
#include <git2/commit.h>
#include <git2/errors.h>
#include <git2/index.h>
#include <git2/repository.h>
#include <git2/signature.h>
#include <git2/tree.h>
#include <git2/types.h>
#include <stdexcept>

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

void GitWrapper::init()
{
    if (repositoryPath == "")
    {
        throw std::runtime_error("git init error: No repository path set");
    }

    int ret = git_repository_init(&libgitRepo, repositoryPath.c_str(), 0);
    if (ret < 0)
    {
        std::runtime_error(std::string("git init error on git_repository_init: ") + git_error_last()->message);
    }
}

void GitWrapper::open()
{
    if (repositoryPath == "")
    {
        std::runtime_error(std::string("git open error: no repository path set"));
    }

    int error = git_repository_open(&libgitRepo, repositoryPath.c_str());

    if (error < 0)
    {
        std::runtime_error(std::string("git open error on git_repository_open: ") + git_error_last()->message);
    }
}

void GitWrapper::add(std::string filenameRelativeFromDirectory)
{
    if (repositoryPath == "")
    {
        throw std::runtime_error("git error on add: No repository set");
    }

    // example: https://libgit2.org/libgit2/ex/HEAD/add.html

    // get the repository index in memory
    git_index *index;
    int ret = git_repository_index(&index, libgitRepo);
    if (ret != 0)
    {
        throw std::runtime_error(std::string("git add error on get_repository_index: ") + git_error_last()->message);
    }

    // create array of files with our file to add
    char *paths[] = {filenameRelativeFromDirectory.data()};
    git_strarray file = {paths, 1};

    // add file to index
    ret = git_index_add_all(index, &file, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr);
    if (ret != 0)
    {
        git_index_free(index);
        throw std::runtime_error(std::string("git add error on git_index_add_all: ") + git_error_last()->message);
    }

    // write back the index to disk
    ret = git_index_write(index);
    if (ret != 0)
    {
        git_index_free(index);
        throw std::runtime_error(std::string("git add error on git_index_write: ") + git_error_last()->message);
    }

    // free the only malloc'd item here (by git_repository_index)
    git_index_free(index);
}

void GitWrapper::commit(std::string commitMessage)
{
    if (repositoryPath == "")
    {
        throw std::runtime_error("git commit error: No repository set");
    }
    // example: https://libgit2.org/libgit2/ex/HEAD/commit.html

    // get the repository index in memory
    git_index *index;
    int ret = git_repository_index(&index, libgitRepo);
    if (ret != 0)
    {
        throw std::runtime_error(std::string("git commit error on get_repository_index: ") + git_error_last()->message);
    }

    // get parent commit and reference to HEAD
    git_object *parent = nullptr;
    git_reference *ref = nullptr;
    ret = git_revparse_ext(&parent, &ref, libgitRepo, "HEAD");
    // we ignore GIT_ENOTFOUND when HEAD is not found because it then
    // means that it's the first commit
    if (ret != GIT_ENOTFOUND && ret != 0)
    {
        throw std::runtime_error(std::string("git commit error on git_revparse_ext: ") + git_error_last()->message);
    }

    // get the root tree oid of the trees in the index
    git_oid tree_oid;
    ret = git_index_write_tree(&tree_oid, index);
    if (ret != 0)
    {
        git_index_free(index);
        if (parent != nullptr)
        {
            git_object_free(parent);
            git_reference_free(ref);
        }
        throw std::runtime_error(std::string("git commit error on git_index_write_tree: ") + git_error_last()->message);
    }

    // write back the index
    ret = git_index_write(index);
    if (ret != 0)
    {
        git_index_free(index);
        if (parent != nullptr)
        {
            git_object_free(parent);
            git_reference_free(ref);
        }
        throw std::runtime_error(std::string("git commit error on git_index_write: ") + git_error_last()->message);
    }

    // fetch the tree we got iod from
    git_tree *tree;
    ret = git_tree_lookup(&tree, libgitRepo, &tree_oid);
    if (ret != 0)
    {
        git_index_free(index);
        if (parent != nullptr)
        {
            git_object_free(parent);
            git_reference_free(ref);
        }
        throw std::runtime_error(std::string("git commit error on git_tree_lookup: ") + git_error_last()->message);
    }

    // sign the repository
    git_signature *signature;
    ret = git_signature_now(&signature, sharedConfig->getName().c_str(), sharedConfig->getMail().c_str());
    if (ret != 0)
    {
        git_index_free(index);
        git_tree_free(tree);
        if (parent != nullptr)
        {
            git_object_free(parent);
            git_reference_free(ref);
        }
        throw std::runtime_error(std::string("git commit error on git_signature_now: ") + git_error_last()->message);
    }

    // create the actual commit and get its oid
    git_oid commit_oid;
    ret = git_commit_create_v(&commit_oid, libgitRepo, "HEAD", signature, signature, nullptr, commitMessage.c_str(),
                              tree, parent ? 1 : 0, parent);
    if (ret != 0)
    {
        git_index_free(index);
        git_signature_free(signature);
        git_tree_free(tree);
        if (parent != nullptr)
        {
            git_object_free(parent);
            git_reference_free(ref);
        }
        throw std::runtime_error(std::string("git commit error on git_commit_create_v: ") + git_error_last()->message);
    }

    git_index_free(index);
    git_signature_free(signature);
    git_tree_free(tree);
    if (parent != nullptr)
    {
        git_object_free(parent);
        git_reference_free(ref);
    }
}

std::string GitWrapper::getBranch()
{
    if (repositoryPath == "" || libgitRepo == nullptr)
    {
        return "master";
    }

    git_reference *head = nullptr;
    const char *branch = nullptr;

    int ret = git_repository_head(&head, libgitRepo);

    // if it's not initialized, let's say we're on master
    if (ret == GIT_EUNBORNBRANCH || ret == GIT_ENOTFOUND)
    {
        return "master";
    }

    if (ret != 0)
    {
        throw std::runtime_error(std::string("git error on git_repository_head: ") + git_error_last()->message);
    }

    branch = git_reference_shorthand(head);
    git_reference_free(head);

    return branch;
}