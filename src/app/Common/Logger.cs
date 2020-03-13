namespace golddrive
{
    public class Logger
    {
        private static NLog.Logger _logger = NLog.LogManager.GetCurrentClassLogger();
        public static void Log(string message)
        {
            _logger.Info(message);
        }
    }
}
