#ifndef DEF_GIT_WRAPPER_HPP
#define DEF_GIT_WRAPPER_HPP

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
     * @brief      Sets the working directory.
     *
     * @param[in]  path  The path
     */
    void setWorkingDirectory(std::string path);

    /**
     * @brief      Initializes the git repository in this working
     *             directory. (similar to git init)
     */
    void init();

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
};

#endif // DEF_GIT_WRAPPER_HPP