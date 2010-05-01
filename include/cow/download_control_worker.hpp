#ifndef ___libcow_download_control_worker___
#define ___libcow_download_control_worker___

namespace libcow 
{
    class download_control_worker
    {
    public:
        download_control_worker();
        ~download_control_worker();
    private:

        dispatcher disp_;
    };
}

#endif // ___libcow_download_control_worker___