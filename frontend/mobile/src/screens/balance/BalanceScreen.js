import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  RefreshControl,
  TouchableOpacity,
  Alert,
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Ionicons } from '@expo/vector-icons';
import { useFocusEffect } from '@react-navigation/native';

import { userAPI } from '../../services/apiService';

const BalanceScreen = ({ navigation }) => {
  const [balanceData, setBalanceData] = useState(null);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);

  const loadBalance = async () => {
    try {
      const response = await userAPI.getBalance();
      if (response.success) {
        setBalanceData(response.data);
      } else {
        Alert.alert('Error', response.error);
      }
    } catch (error) {
      Alert.alert('Error', 'Failed to load balance');
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  };

  useEffect(() => {
    loadBalance();
  }, []);

  useFocusEffect(
    useCallback(() => {
      if (!loading) {
        loadBalance();
      }
    }, [loading])
  );

  const handleRefresh = () => {
    setRefreshing(true);
    loadBalance();
  };

  const formatBalance = (amount) => {
    return Math.abs(amount).toFixed(2);
  };

  const getBalanceColor = (amount) => {
    if (amount > 0) return '#38A169';
    if (amount < 0) return '#E53E3E';
    return '#718096';
  };

  const getBalanceSign = (amount) => {
    if (amount > 0) return '+';
    if (amount < 0) return '-';
    return '';
  };

  const renderEventBalance = (eventBalance) => (
    <View key={eventBalance.event_id} style={styles.eventBalanceCard}>
      <View style={styles.eventInfo}>
        <Text style={styles.eventName} numberOfLines={1}>
          {eventBalance.event_name}
        </Text>
        <Text style={[
          styles.eventBalance,
          { color: getBalanceColor(eventBalance.balance) }
        ]}>
          {getBalanceSign(eventBalance.balance)}${formatBalance(eventBalance.balance)}
        </Text>
      </View>
      <TouchableOpacity
        style={styles.viewEventButton}
        onPress={() => navigation.navigate('Events', { 
          screen: 'EventDetails', 
          params: { eventId: eventBalance.event_id } 
        })}
      >
        <Ionicons name="chevron-forward" size={16} color="#A0AEC0" />
      </TouchableOpacity>
    </View>
  );

  if (loading) {
    return (
      <SafeAreaView style={styles.container}>
        <View style={styles.header}>
          <Text style={styles.title}>Balance</Text>
        </View>
        <View style={styles.loadingContainer}>
          <Text style={styles.loadingText}>Loading balance...</Text>
        </View>
      </SafeAreaView>
    );
  }

  const totalBalance = balanceData?.total_balance || 0;
  const eventBalances = balanceData?.event_balances || [];

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Balance</Text>
      </View>

      <ScrollView
        style={styles.content}
        refreshControl={
          <RefreshControl
            refreshing={refreshing}
            onRefresh={handleRefresh}
            tintColor="#1A365D"
          />
        }
        showsVerticalScrollIndicator={false}
      >
        {/* Total Balance Card */}
        <View style={styles.totalBalanceCard}>
          <Text style={styles.totalBalanceLabel}>Total Balance</Text>
          <Text style={[
            styles.totalBalanceAmount,
            { color: getBalanceColor(totalBalance) }
          ]}>
            {getBalanceSign(totalBalance)}${formatBalance(totalBalance)}
          </Text>
          <Text style={styles.totalBalanceDescription}>
            {totalBalance > 0 && "You're owed money overall"}
            {totalBalance < 0 && "You owe money overall"}
            {totalBalance === 0 && "You're all settled up"}
          </Text>
        </View>

        {/* Event Balances */}
        {eventBalances.length > 0 && (
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>By Event</Text>
            {eventBalances.map(renderEventBalance)}
          </View>
        )}

        {/* Summary */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Summary</Text>
          <View style={styles.summaryCard}>
            <View style={styles.summaryRow}>
              <Text style={styles.summaryLabel}>Active Events</Text>
              <Text style={styles.summaryValue}>{eventBalances.length}</Text>
            </View>
            <View style={styles.summaryRow}>
              <Text style={styles.summaryLabel}>Events Settled</Text>
              <Text style={styles.summaryValue}>
                {eventBalances.filter(e => e.balance === 0).length}
              </Text>
            </View>
            <View style={styles.summaryRow}>
              <Text style={styles.summaryLabel}>Money in Transit</Text>
              <Text style={styles.summaryValue}>
                ${eventBalances.reduce((sum, e) => sum + Math.abs(e.balance), 0).toFixed(2)}
              </Text>
            </View>
          </View>
        </View>

        {eventBalances.length === 0 && (
          <View style={styles.emptyState}>
            <Ionicons name="wallet-outline" size={64} color="#A0AEC0" />
            <Text style={styles.emptyTitle}>No Active Balances</Text>
            <Text style={styles.emptySubtitle}>
              Create an event and add expenses to see your balance
            </Text>
          </View>
        )}
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#FAFAFA',
  },
  header: {
    paddingHorizontal: 16,
    paddingVertical: 16,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E2E8F0',
  },
  title: {
    fontSize: 24,
    fontWeight: '700',
    color: '#2D3748',
  },
  content: {
    flex: 1,
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    fontSize: 16,
    color: '#718096',
  },
  totalBalanceCard: {
    backgroundColor: '#FFFFFF',
    margin: 16,
    padding: 24,
    borderRadius: 12,
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.05,
    shadowRadius: 3,
    elevation: 2,
  },
  totalBalanceLabel: {
    fontSize: 16,
    color: '#718096',
    marginBottom: 8,
  },
  totalBalanceAmount: {
    fontSize: 36,
    fontWeight: '700',
    marginBottom: 8,
  },
  totalBalanceDescription: {
    fontSize: 14,
    color: '#718096',
    textAlign: 'center',
  },
  section: {
    marginHorizontal: 16,
    marginBottom: 24,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '600',
    color: '#2D3748',
    marginBottom: 12,
  },
  eventBalanceCard: {
    backgroundColor: '#FFFFFF',
    padding: 16,
    borderRadius: 8,
    marginBottom: 8,
    flexDirection: 'row',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    elevation: 1,
  },
  eventInfo: {
    flex: 1,
  },
  eventName: {
    fontSize: 16,
    fontWeight: '500',
    color: '#2D3748',
    marginBottom: 2,
  },
  eventBalance: {
    fontSize: 14,
    fontWeight: '600',
  },
  viewEventButton: {
    padding: 8,
  },
  summaryCard: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 16,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.05,
    shadowRadius: 2,
    elevation: 1,
  },
  summaryRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  summaryLabel: {
    fontSize: 14,
    color: '#718096',
  },
  summaryValue: {
    fontSize: 14,
    fontWeight: '600',
    color: '#2D3748',
  },
  emptyState: {
    alignItems: 'center',
    paddingHorizontal: 32,
    paddingTop: 48,
  },
  emptyTitle: {
    fontSize: 20,
    fontWeight: '600',
    color: '#2D3748',
    marginTop: 16,
    marginBottom: 8,
  },
  emptySubtitle: {
    fontSize: 16,
    color: '#718096',
    textAlign: 'center',
    lineHeight: 24,
  },
});

export default BalanceScreen;