const { isNightTarif } = require('./IsNightTarif');
const assert = require('assert');


describe('IsNightTarif', function() {
    it('should return true for night hours', function () {
        // 01/01/2019 @ 03:00 (UTC)
      	assert.equal(isNightTarif(new Date(1546286400000)), true);
    });

    it('should return false for day hours', function () {
        // 01/01/2019 @ 13:00 (UTC)
        assert.equal(isNightTarif(new Date(1546322400000)), false);
    });

    it('should return true for weekends', function () {
        // 01/05/2019  @ 13:00 (UTC) -> Saturday
        assert.equal(isNightTarif(new Date(1546668000000)), true);
    });

    it('should also work when we pass integers instead of date objects', function () {
         // 01/05/2019  @ 13:00 (UTC) -> Saturday
         assert.equal(isNightTarif(1546668000), true);
    });
});