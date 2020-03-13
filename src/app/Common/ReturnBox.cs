namespace golddrive
{
    public class ReturnBox
    {
        public ReturnBox()
        {
            MountStatus = MountStatus.UNKNOWN;
            DriveStatus = DriveStatus.UNKNOWN;
            Success = false;
            ExitCode = -999;
        }
        public Drive Drive { get; set; }
        public string Error { get; set; }
        public string Output { get; set; }
        public int ExitCode { get; set; }
        public bool Success { get; set; }
        public MountStatus MountStatus { get; set; }
        public DriveStatus DriveStatus { get; set; }
    }
}
