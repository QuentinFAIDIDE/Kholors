#ifndef DEF_GIT_WRAPPER_HPP
#define DEF_GIT_WRAPPER_HPP

#include <git2.h>
#include <git2/repository.h>
#include <string>

/**
 * @brief      This class describes a git wrapper.
 *             It will initialize git repositories, add file
 *             commit them, switch branches, and eventually
 *             push to a remote one day.
 */
class GitWrapper
{
  public:
    /**
     * @brief      Constructs a new instance.
     */
    GitWrapper();

    /**
     * @brief      Destroys the object.
     */
    ~GitWrapper();

    /**
     * @brief      Sets the working directory.
     *
     * @param[in]  path  The path
     */
    void setWorkingDirectory(std::string path);

    /**
     * @brief      Initializes the git repository in this working
     *             directory. (similar to git init).
     *             Return an empty string if no error, or the error message
     *             otherwise.
     */
    std::string init();

    /**
     * @brief      Open git repository at current path.
     *
     * @return     The error or an empty string if there were none.
     */
    std::string open();

    /**
     * @brief      Add a file to the current commit (similar to git add).
     *
     * @param[in]  filenameRelativeFromDirectory  The filename relative from directory
     */
    void add(std::string filenameRelativeFromDirectory);

    /**
     * @brief      Commit files added. Similar to a git commit.
     *
     * @param[in]  commitMessage  The commit message
     */
    void commit(std::string commitMessage);

    /**
     * @brief      Gets the branch the repo on the working directory is on.
     *
     * @return     The branch.
     */
    std::string getBranch();

  private:
    std::string repositoryPath;
    git_repository *libgitRepo;
};

#endif // DEF_GIT_WRAPPER_HPP